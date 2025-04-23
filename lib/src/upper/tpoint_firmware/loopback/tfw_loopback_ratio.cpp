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

#include "dectnrp/common/adt/freq_shift.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/external/nlohmann/json.hpp"

namespace dectnrp::upper::tfw::loopback {

const std::string tfw_loopback_ratio_t::firmware_name("loopback_ratio");

tfw_loopback_ratio_t::tfw_loopback_ratio_t(const tpoint_config_t& tpoint_config_,
                                           phy::mac_lower_t& mac_lower_)
    : tfw_loopback_t(tpoint_config_, mac_lower_) {
    mcs_index_start = 1;
    mcs_index_end = 2;
    mcs_index = mcs_index_start;
    mcs_cnt = 0;

    for (uint32_t i = mcs_index_start; i <= mcs_index_end; ++i) {
        PER_pcc_crc.push_back(std::vector<float>());
        PER_pcc_crc_and_plcf.push_back(std::vector<float>());
        PER_pdc_crc.push_back(std::vector<float>());
    }

    packet_tx_time_multiple = 1;

    reset_result_counter_for_next_snr();
}

phy::maclow_phy_t tfw_loopback_ratio_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    ++n_pcc_crc;

    // base pointer to extract PLCF_type
    const auto* plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(PLCF_type);

    // is this the correct PLCF type?
    if (plcf_base == nullptr) {
        return phy::maclow_phy_t();
    }

    // is this the correct header type?
    if (plcf_base->get_HeaderFormat() != PLCF_type_header_format) {
        return phy::maclow_phy_t();
    }

    ++n_pcc_crc_and_plcf;

    return worksub_pcc2pdc(phy_maclow,
                           PLCF_type,
                           identity.NetworkID,
                           0,
                           phy::harq::finalize_rx_t::reset_and_terminate,
                           phy::maclow_phy_handle_t());
}

phy::machigh_phy_t tfw_loopback_ratio_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    if (phy_machigh.pdc_report.crc_status) {
        ++n_pdc_crc;

        snr_max = std::max(snr_max, phy_machigh.pdc_report.snr_dB);
        snr_min = std::min(snr_min, phy_machigh.pdc_report.snr_dB);
    }

    return phy::machigh_phy_t();
}

void tfw_loopback_ratio_t::reset_result_counter_for_next_snr() {
    nof_experiment_cnt = 0;

    n_pcc_crc = 0;
    n_pcc_crc_and_plcf = 0;
    n_pdc_crc = 0;

    snr_max = -100.0e6;
    snr_min = -snr_max;
}

void tfw_loopback_ratio_t::generate_single_experiment_at_current_snr(
    const int64_t now_64, phy::machigh_phy_t& machigh_phy) {
    // update MCS
    psdef.mcs_index = mcs_index;
    plcf_10.DFMCS = psdef.mcs_index;
    plcf_20.DFMCS = psdef.mcs_index;
    plcf_21.DFMCS = psdef.mcs_index;

    // find next possible TX time
    int64_t tx_time_64 = now_64 + hw.get_tmin_samples(radio::hw_t::tmin_t::turnaround);

    // add a random jitter
    tx_time_64 += static_cast<int64_t>(randomgen.randi(
        0,
        static_cast<uint32_t>(
            duration_lut.get_N_samples_from_duration(section3::duration_ec_t::subslot_u1_001))));

    // force transmission time to the next multiple of packet_tx_time_multiple
    const int64_t res = tx_time_64 % packet_tx_time_multiple;
    if (res != 0) {
        tx_time_64 += packet_tx_time_multiple - res;
    }

    generate_packet(tx_time_64, machigh_phy);
}

void tfw_loopback_ratio_t::save_result_of_current_snr() {
    const float per_pcc_crc = 1.0f - float(n_pcc_crc) / float(nof_experiment);
    const float per_pcc_crc_and_plcf = 1.0f - float(n_pcc_crc_and_plcf) / float(nof_experiment);
    const float per_pdc_crc = 1.0f - float(n_pdc_crc) / float(nof_experiment);

    PER_pcc_crc[mcs_cnt].push_back(per_pcc_crc);
    PER_pcc_crc_and_plcf[mcs_cnt].push_back(per_pcc_crc_and_plcf);
    PER_pdc_crc[mcs_cnt].push_back(per_pdc_crc);

    dectnrp_log_inf(
        "MCS={} SNR={} nof_experiment={} | per_pcc_crc={} per_pcc_crc_and_plcf={} per_pdc_crc={} | snr_max={} snr_min={}",
        mcs_index,
        snr,
        nof_experiment,
        per_pcc_crc,
        per_pcc_crc_and_plcf,
        per_pdc_crc,
        snr_max,
        snr_min);
}

bool tfw_loopback_ratio_t::set_next_parameter_or_go_to_dead_end() {
    ++mcs_index;
    ++mcs_cnt;

    if (mcs_index > mcs_index_end) {
        return true;
    }

    return false;
}

void tfw_loopback_ratio_t::save_all_results_to_file() const {
    // create SNR vector
    std::vector<float> snr_vec;
    for (float snr_ = snr_start; snr_ <= snr_stop; snr_ += snr_step) {
        snr_vec.push_back(snr_);
    }

    // save one file for every MCS
    uint32_t mcs_cnt_local = 0;
    for (uint32_t i = mcs_index_start; i <= mcs_index_end; ++i) {
        // save all data to json file
        std::ostringstream filename;
        filename << "rx_loopback_MCS_" << std::setw(4) << std::setfill('0') << i;

        nlohmann::ordered_json j_packet_data;

        j_packet_data["nof_experiment"] = nof_experiment;
        j_packet_data["MCS_index"] = i;

        j_packet_data["data"]["snr_vec"] = snr_vec;
        j_packet_data["data"]["PER_pcc_crc"] = PER_pcc_crc[mcs_cnt_local];
        j_packet_data["data"]["PER_pcc_crc_and_plcf"] = PER_pcc_crc_and_plcf[mcs_cnt_local];
        j_packet_data["data"]["PER_pdc_crc"] = PER_pdc_crc[mcs_cnt_local];

        std::ofstream out_file(filename.str());
        out_file << std::setw(4) << j_packet_data << std::endl;
        out_file.close();

        ++mcs_cnt_local;
    }
}

}  // namespace dectnrp::upper::tfw::loopback
