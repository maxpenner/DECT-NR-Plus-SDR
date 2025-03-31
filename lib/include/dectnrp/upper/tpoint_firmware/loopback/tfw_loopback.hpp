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

#pragma once

#include "dectnrp/common/multidim.hpp"
#include "dectnrp/common/randomgen.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::loopback {

class tfw_loopback_t final : public tpoint_t {
    public:
        tfw_loopback_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_loopback_t() = default;

        tfw_loopback_t() = delete;
        tfw_loopback_t(const tfw_loopback_t&) = delete;
        tfw_loopback_t& operator=(const tfw_loopback_t&) = delete;
        tfw_loopback_t(tfw_loopback_t&&) = delete;
        tfw_loopback_t& operator=(tfw_loopback_t&&) = delete;

        static const std::string firmware_name;

        void work_start_imminent(const int64_t start_time_64) override;
        phy::machigh_phy_t work_regular(const phy::phy_mac_reg_t& phy_mac_reg) override;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override;
        phy::machigh_phy_t work_pdc_async(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_upper(const upper::upper_report_t& upper_report) override;
        phy::machigh_phy_tx_t work_chscan_async(const phy::chscan_t& chscan) override;

    private:
        std::vector<std::string> start_threads() override final;
        std::vector<std::string> stop_threads() override final;

        // ##################################################
        // Radio Layer + PHY

        radio::hw_simulator_t* hw_simulator;

        // ##################################################
        // MAC Layer

        section4::mac_architecture::identity_t identity;
        uint32_t ShortRadioDeviceID_rx;
        uint32_t PLCF_type;
        uint32_t PLCF_type_header_format;

        // ##################################################
        // measurement logic

        enum class STATE_t {
            SET_SNR,
            SET_SMALL_SCALE_FADING,
            GENERATE_PACKET,
            SAVE_RESULTS,
            SETUP_NEXT_MEASUREMENT,
            DEAD_END
        } state;

        /// timing between states
        int64_t state_time_reference_64;

        // ##################################################
        // What do we measure?

        /// we measure PER over SNR ....
        float snr_start;
        float snr_step;
        float snr_stop;
        float snr;

        /// ... and MCS
        uint32_t mcs_index_start;
        uint32_t mcs_index_end;
        uint32_t mcs_index;  // current MCS index, first value can be >0
        uint32_t mcs_cnt;    // current MCS counter, always starts with 0

        /// at every SNR and each MCS, we measure the same number of packets
        uint32_t nof_packets;
        uint32_t nof_packets_cnt;

        /// At every SNR and each MCS, we count CRC successes. We don't compare bit by bit.
        uint32_t n_pcc_crc;
        uint32_t n_pcc_crc_and_plcf;
        uint32_t n_pdc_crc;

        /// at every SNR and each MCS, we also save the measured SNR of the PDC
        float snr_max;
        float snr_min;

        /// to later save the results in a file, we write them to these containers
        std::vector<uint32_t> TB_bits;
        common::vec2d<float> PER_pcc_crc;
        common::vec2d<float> PER_pcc_crc_and_plcf;
        common::vec2d<float> PER_pdc_crc;

        // ##################################################
        // additional debugging

        /// for transmission time to a specific sample multiple
        int64_t packet_tx_time_multiple;

        /// fractional CFO is two subcarriers
        float cfo_symmetric_range_subc_multiple;

        // ##################################################
        // utilities

        common::randomgen_t randomgen;
        void reset_for_new_run();
        void generate_one_new_packet(const int64_t now_64, phy::machigh_phy_t& machigh_phy);
        void save_to_files() const;
};

}  // namespace dectnrp::upper::tfw::loopback
