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

#include "dectnrp/application/application_client.hpp"
#include "dectnrp/application/application_server.hpp"
#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
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

        phy::irregular_report_t work_start(const int64_t start_time_64) override;
        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override;
        phy::machigh_phy_t work_pdc(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_pdc_error(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override;
        phy::machigh_phy_tx_t work_channel(const phy::chscan_t& chscan) override;
        void work_stop() override final;

    private:
        /// number of transmitted and received packets per run
        uint32_t N_measurement_tx_cnt{0};
        uint32_t N_measurement_rx_cnt{0};

        /// measured round trip time from MAC to MAC
        int64_t rtt_min{-common::adt::UNDEFINED_EARLY_64};
        int64_t rtt_max{common::adt::UNDEFINED_EARLY_64};

        /// operating system clock to measure rtt
        common::watch_t watch;

        /// packet dimensions
        sp3::packet_sizes_def_t psdef;

        /// last time the AGC was tuned
        int64_t t_agc_xx_last_change_64{common::adt::UNDEFINED_EARLY_64};

        /// next time AGC will be tuned
        int64_t t_agc_tx_change_64{common::adt::UNDEFINED_EARLY_64};
        int64_t t_agc_rx_change_64{common::adt::UNDEFINED_EARLY_64};

        /// FT and PT must know both identities
        sp4::mac_architecture::identity_t identity_ft;
        sp4::mac_architecture::identity_t identity_pt;

        /// required to tune the AGC in an irregular callback
        phy::sync_report_t sync_report;

        /// PLCF fixed to type 1 and header format 0
        sp4::plcf_10_t plcf_10_tx;
        sp4::plcf_10_t plcf_10_rx;

        /// FT receives data from application layer, and forwards data to application layer
        std::unique_ptr<application::application_server_t> application_server;
        std::unique_ptr<application::application_client_t> application_client;

        /// working copy to transfer payloads
        std::vector<uint8_t> stage_a;

        /// used at FT and PT
        void generate_packet_asap(phy::machigh_phy_t& machigh_phy);

        phy::machigh_phy_t work_pdc_internal(const phy::phy_machigh_t& phy_machigh);
};

}  // namespace dectnrp::upper::tfw::rtt
