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

#include "dectnrp/sections_part4/mac_messages_and_ie/cluster_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions//time_announce_ie.hpp"
#include "dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_base.hpp"

#define TFW_P2P_PT_AGC_ENABLED
#define TFW_P2P_PT_AGC_CHANGE_TIMED_OR_IMMEDIATE_PT

namespace dectnrp::upper::tfw::p2p {

class tfw_p2p_pt_t final : public tfw_p2p_base_t {
    public:
        explicit tfw_p2p_pt_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_p2p_pt_t() = default;

        tfw_p2p_pt_t() = delete;
        tfw_p2p_pt_t(const tfw_p2p_pt_t&) = delete;
        tfw_p2p_pt_t& operator=(const tfw_p2p_pt_t&) = delete;
        tfw_p2p_pt_t(tfw_p2p_pt_t&&) = delete;
        tfw_p2p_pt_t& operator=(tfw_p2p_pt_t&&) = delete;

        static const std::string firmware_name;

        phy::irregular_report_t work_start_imminent(const int64_t start_time_64) override final;
        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override final;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;

        // work_pcc() and work_pdc_async() are implemented in deriving classes

        phy::machigh_phy_t work_upper(const upper::upper_report_t& upper_report) override final;
        phy::machigh_phy_tx_t work_chscan_async(const phy::chscan_t& chscan) override final;

    private:
        std::vector<std::string> start_threads() override final;
        std::vector<std::string> stop_threads() override final;

        // ##################################################
        // Radio Layer + PHY

        void init_radio() override final;
        void init_simulation_if_detected() override final;

        // ##################################################
        // MAC Layer

        /**
         * \brief The FT saves a full contact_list_t with one entry per PT. A PT saves a single
         * contact with information about itself.
         */
        contact_p2p_t contact_pt;

        // clang-format off
        std::optional<phy::maclow_phy_t> worksub_pcc_10(const phy::phy_maclow_t& phy_maclow) override final;
        phy::maclow_phy_t worksub_pcc_20(const phy::phy_maclow_t& phy_maclow) override final;
        phy::maclow_phy_t worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) override final;
        
        phy::machigh_phy_t worksub_pdc_10(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t worksub_pdc_20(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) override final;
        // clang-format on

        void worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) override final;

        void worksub_mmie_cluster_beacon_message(
            const phy::phy_machigh_t& phy_machigh,
            const sp4::cluster_beacon_message_t& cluster_beacon_message);

        void worksub_mmie_time_announce(
            const phy::phy_machigh_t& phy_machigh,
            const sp4::extensions::time_announce_ie_t& time_announce_ie);

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer

        void init_appiface() override final;

        // ##################################################
        // logging

        void worksub_callback_log(const int64_t now_64) const override final;
};

}  // namespace dectnrp::upper::tfw::p2p
