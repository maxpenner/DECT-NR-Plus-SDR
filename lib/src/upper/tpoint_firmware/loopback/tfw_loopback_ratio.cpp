/*
 * Copyright 2023-2025 Maxim Penner
 *
 * This file is part of DECTNRP.
 *
 * DECTNRP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * DECTNRP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 */

#include "dectnrp/upper/tpoint_firmware/loopback/tfw_loopback_ratio.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "header_only/nlohmann/json.hpp"

namespace dectnrp::upper::tfw::loopback {

const std::string tfw_loopback_ratio_t::firmware_name("loopback_ratio");

tfw_loopback_ratio_t::tfw_loopback_ratio_t(const tpoint_config_t& tpoint_config_,
                                           phy::mac_lower_t& mac_lower_)
    : tfw_loopback_t(tpoint_config_, mac_lower_) {
    // clear and overwrite snr vector
    snr_vec.clear();
    for (float snr = 0.0f; snr <= 10.0f; snr += 1.0f) {
        snr_vec.push_back(snr);
    }
    nof_experiment_per_snr = 100;

    ratio_vec = std::vector<int32_t>{30, 40, 50, 60, 70, 80};

    identity_A = section4::mac_architecture::identity_t(100, 10000000, 1000);
    identity_B = section4::mac_architecture::identity_t(123, 12345678, 1234);

    result = result_t(ratio_vec.size(), snr_vec.size());
}

phy::maclow_phy_t tfw_loopback_ratio_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    ++result.n_pcc;

    // base pointer to extract PLCF_type
    const auto* plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(pp.PLCF_type);

    // is this the correct PLCF type?
    if (plcf_base == nullptr) {
        return phy::maclow_phy_t();
    }

    // is this the correct header type?
    if (plcf_base->get_HeaderFormat() != pp.PLCF_type_header_format) {
        return phy::maclow_phy_t();
    }

    // is this the correct short radio device ID?
    if (pp.PLCF_type == 1) {
        // cast guaranteed to work
        const auto* plcf_10 = static_cast<const section4::plcf_10_t*>(plcf_base);

        if (plcf_10->TransmitterIdentity != pp.identity.ShortRadioDeviceID) {
            return phy::maclow_phy_t();
        }
    } else {
        if (pp.PLCF_type_header_format == 0) {
            // cast guaranteed to work
            const auto* plcf_20 = static_cast<const section4::plcf_20_t*>(plcf_base);

            if (plcf_20->TransmitterIdentity != identity_B.ShortRadioDeviceID) {
                return phy::maclow_phy_t();
            }
        } else {
            // cast guaranteed to work
            const auto* plcf_21 = static_cast<const section4::plcf_21_t*>(plcf_base);

            if (plcf_21->TransmitterIdentity != identity_B.ShortRadioDeviceID) {
                return phy::maclow_phy_t();
            }
        }
    }

    ++result.n_pcc_and_plcf;

    return worksub_pcc2pdc(phy_maclow,
                           pp.PLCF_type,
                           identity_B.NetworkID,
                           0,
                           phy::harq::finalize_rx_t::reset_and_terminate,
                           phy::maclow_phy_handle_t());
}

phy::machigh_phy_t tfw_loopback_ratio_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    if (phy_machigh.pdc_report.crc_status) {
        ++result.n_pdc;

        result.overwrite_or_discard_snr_max(parameter_cnt, snr_cnt, phy_machigh.pdc_report.snr_dB);
        result.overwrite_or_discard_snr_min(parameter_cnt, snr_cnt, phy_machigh.pdc_report.snr_dB);
    }

    return phy::machigh_phy_t();
}

void tfw_loopback_ratio_t::A_reset_result_counter_for_next_snr() {
    nof_experiment_per_snr_cnt = 0;

    result.reset();
}

void tfw_loopback_ratio_t::C_generate_single_experiment_at_current_snr(
    const int64_t now_64, phy::machigh_phy_t& machigh_phy) {
    const int64_t tx_time_64 = get_random_tx_time(now_64);

    // set first packet
    pp.identity = identity_A;
    pp.update_plcf_unpacked();
    pp.tx_time_64 = tx_time_64;
    pp.amplitude_scale = common::adt::db2mag(static_cast<float>(ratio_vec.at(parameter_cnt)));

    // generate first packet
    generate_packet(machigh_phy);

    // set first packet
    pp.identity = identity_B;
    pp.update_plcf_unpacked();
    pp.tx_time_64 = tx_time_64 + static_cast<int64_t>(pp.N_samples_in_packet_length);
    pp.amplitude_scale = 1.0f;

    // generate second packet right after the first one
    generate_packet(machigh_phy);
}

void tfw_loopback_ratio_t::D_save_result_of_current_snr() {
    result.set_PERs(parameter_cnt, snr_cnt, nof_experiment_per_snr);

    dectnrp_log_inf(
        "ratio={} SNR={} | per_pcc_crc={} per_pcc_crc_and_plcf={} per_pdc_crc={} | snr_max={} snr_min={}",
        ratio_vec.at(parameter_cnt),
        snr_vec.at(snr_cnt),
        result.PER_pcc.at(parameter_cnt).at(snr_cnt),
        result.PER_pcc_and_plcf.at(parameter_cnt).at(snr_cnt),
        result.PER_pdc.at(parameter_cnt).at(snr_cnt),
        result.snr_max_vec.at(parameter_cnt).at(snr_cnt),
        result.snr_min_vec.at(parameter_cnt).at(snr_cnt));
}

bool tfw_loopback_ratio_t::E_set_next_parameter_or_go_to_dead_end() {
    ++parameter_cnt;

    if (parameter_cnt >= ratio_vec.size()) {
        return true;
    }

    return false;
}

void tfw_loopback_ratio_t::save_all_results_to_file() const {
    // save one file for every power ratio
    uint32_t parameter_cnt_local = 0;
    for (const auto ratio : ratio_vec) {
        std::ostringstream filename;
        filename << "rx_loopback_ratio_" << std::setw(3) << std::setfill('0') << parameter_cnt_local
                 << "_" << ratio;

        nlohmann::ordered_json j_packet_data;

        j_packet_data["experiment_range"]["snr_vec"] = snr_vec;
        j_packet_data["experiment_range"]["nof_experiment_per_snr"] = nof_experiment_per_snr;

        j_packet_data["parameter"]["ratio"] = ratio;

        // clang-format off
        j_packet_data["result"]["snr_max_vec"] = result.snr_max_vec[parameter_cnt_local];
        j_packet_data["result"]["snr_min_vec"] = result.snr_min_vec[parameter_cnt_local];
        j_packet_data["result"]["PER_pcc_crc"] = result.PER_pcc[parameter_cnt_local];
        j_packet_data["result"]["PER_pcc_crc_and_plcf"] = result.PER_pcc_and_plcf[parameter_cnt_local];
        j_packet_data["result"]["PER_pdc_crc"] = result.PER_pdc[parameter_cnt_local];
        // clang-format on

        std::ofstream out_file(filename.str());
        out_file << std::setw(4) << j_packet_data << std::endl;
        out_file.close();

        ++parameter_cnt_local;
    }
}

}  // namespace dectnrp::upper::tfw::loopback
