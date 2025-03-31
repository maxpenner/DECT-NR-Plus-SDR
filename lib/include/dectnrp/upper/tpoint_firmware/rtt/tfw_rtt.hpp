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

#include "dectnrp/application/app_client.hpp"
#include "dectnrp/application/app_server.hpp"
#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_pdu_decoder.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::rtt {

class tfw_rtt_t final : public tpoint_t {
    public:
        tfw_rtt_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_rtt_t() = default;

        tfw_rtt_t() = delete;
        tfw_rtt_t(const tfw_rtt_t&) = delete;
        tfw_rtt_t& operator=(const tfw_rtt_t&) = delete;
        tfw_rtt_t(tfw_rtt_t&&) = delete;
        tfw_rtt_t& operator=(tfw_rtt_t&&) = delete;

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

        /// number of transmitted and received packets per print
        uint32_t N_measurement_tx_cnt{0};
        uint32_t N_measurement_rx_cnt{0};

        /// measured round trip time from MAC to MAC
        int64_t rtt_min{-common::adt::UNDEFINED_EARLY_64};
        int64_t rtt_max{common::adt::UNDEFINED_EARLY_64};

        /// measured rms
        float rms_max{-1000.0f};

        /// we use the operating system's clock to measure time
        common::watch_t watch;

        /// packet and PLCF identities (PLCF_type 1, header format 0)
        const uint32_t plcf_type{1};
        const uint32_t N_subslots{2};
        section4::mac_architecture::identity_t identity_ft;
        section4::mac_architecture::identity_t identity_pt;

        /// FT receives data from and forwards data to application layer
        std::unique_ptr<application::app_server_t> app_server;
        std::unique_ptr<application::app_client_t> app_client;

        /// PT receives data in NR+ packets and sends its back to FT
        const section4::mac_pdu_decoder_t* mac_pdu_decoder{nullptr};

        /// size of payload
        uint32_t N_TB_byte;

        /// working copy that we use to insert the measured RTT into the payload
        std::vector<uint8_t> a_raw_with_rtt;

        /// used at FT and PT
        void generate_packet_asap(const section4::mac_architecture::identity_t identity,
                                  phy::machigh_phy_t& machigh_phy);
};

}  // namespace dectnrp::upper::tfw::rtt
