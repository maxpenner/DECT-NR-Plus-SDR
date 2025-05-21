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

#include "dectnrp/upper/tpoint_firmware/loopback/tfw_loopback_snr.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "dectnrp/common/prog/log.hpp"
#include "header_only/nlohmann/json.hpp"

namespace dectnrp::upper::tfw::loopback {

const std::string tfw_loopback_snr_t::firmware_name("loopback_snr");

tfw_loopback_snr_t::tfw_loopback_snr_t(const tpoint_config_t& tpoint_config_,
                                       phy::mac_lower_t& mac_lower_)
    : tfw_loopback_t(tpoint_config_, mac_lower_) {
    // clear and overwrite snr vector
    snr_vec.clear();
    for (float snr = -2.0f; snr <= 20.0f; snr += 1.0f) {
        snr_vec.push_back(snr);
    }
    nof_experiment_per_snr = 100;

    mcs_vec = std::vector<uint32_t>{1, 2, 3, 4, 5, 6};

    result = result_t(mcs_vec.size(), snr_vec.size());
}

phy::maclow_phy_t tfw_loopback_snr_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
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
        const auto* plcf_10 = static_cast<const sp4::plcf_10_t*>(plcf_base);

        if (plcf_10->TransmitterIdentity != pp.identity.ShortRadioDeviceID) {
            return phy::maclow_phy_t();
        }
    } else {
        if (pp.PLCF_type_header_format == 0) {
            // cast guaranteed to work
            const auto* plcf_20 = static_cast<const sp4::plcf_20_t*>(plcf_base);

            if (plcf_20->TransmitterIdentity != pp.identity.ShortRadioDeviceID) {
                return phy::maclow_phy_t();
            }
        } else {
            // cast guaranteed to work
            const auto* plcf_21 = static_cast<const sp4::plcf_21_t*>(plcf_base);

            if (plcf_21->TransmitterIdentity != pp.identity.ShortRadioDeviceID) {
                return phy::maclow_phy_t();
            }
        }
    }

    ++result.n_pcc_and_plcf;

    return worksub_pcc2pdc(phy_maclow,
                           pp.PLCF_type,
                           pp.identity.NetworkID,
                           0,
                           phy::harq::finalize_rx_t::reset_and_terminate,
                           phy::maclow_phy_handle_t());
}

phy::machigh_phy_t tfw_loopback_snr_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    if (phy_machigh.pdc_report.crc_status) {
        ++result.n_pdc;

        result.overwrite_or_discard_snr_max(parameter_cnt, snr_cnt, phy_machigh.pdc_report.snr_dB);
        result.overwrite_or_discard_snr_min(parameter_cnt, snr_cnt, phy_machigh.pdc_report.snr_dB);
    }

    return phy::machigh_phy_t();
}

void tfw_loopback_snr_t::A_reset_result_counter_for_next_snr() {
    nof_experiment_per_snr_cnt = 0;

    result.reset();
}

void tfw_loopback_snr_t::C_generate_single_experiment_at_current_snr(
    const int64_t now_64, phy::machigh_phy_t& machigh_phy) {
    // update MCS
    pp.psdef.mcs_index = mcs_vec.at(parameter_cnt);
    pp.plcf_10.DFMCS = pp.psdef.mcs_index;
    pp.plcf_20.DFMCS = pp.psdef.mcs_index;
    pp.plcf_21.DFMCS = pp.psdef.mcs_index;

    // find next possible TX time
    pp.tx_time_64 = get_random_tx_time(now_64);

    generate_packet(machigh_phy);
}

void tfw_loopback_snr_t::D_save_result_of_current_snr() {
    result.set_PERs(parameter_cnt, snr_cnt, nof_experiment_per_snr);

    dectnrp_log_inf(
        "mcs={} SNR={} | per_pcc_crc={} per_pcc_crc_and_plcf={} per_pdc_crc={} | snr_max={} snr_min={}",
        mcs_vec.at(parameter_cnt),
        snr_vec.at(snr_cnt),
        result.PER_pcc.at(parameter_cnt).at(snr_cnt),
        result.PER_pcc_and_plcf.at(parameter_cnt).at(snr_cnt),
        result.PER_pdc.at(parameter_cnt).at(snr_cnt),
        result.snr_max_vec.at(parameter_cnt).at(snr_cnt),
        result.snr_min_vec.at(parameter_cnt).at(snr_cnt));
}

bool tfw_loopback_snr_t::E_set_next_parameter_or_go_to_dead_end() {
    ++parameter_cnt;

    if (parameter_cnt == mcs_vec.size()) {
        return true;
    }

    return false;
}

void tfw_loopback_snr_t::save_all_results_to_file() const {
    // save one file for every MCS
    uint32_t parameter_cnt_local = 0;
    for (const auto mcs : mcs_vec) {
        std::ostringstream filename;
        filename << "rx_loopback_MCS_" << std::setw(4) << std::setfill('0') << mcs;

        nlohmann::ordered_json j_packet_data;

        j_packet_data["experiment_range"]["snr_vec"] = snr_vec;
        j_packet_data["experiment_range"]["nof_experiment_per_snr"] = nof_experiment_per_snr;

        j_packet_data["parameter"]["mcs"] = mcs;

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
