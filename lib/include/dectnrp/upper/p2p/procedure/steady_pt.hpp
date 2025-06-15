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
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/time_announce_ie.hpp"
#include "dectnrp/upper/p2p/data/pt.hpp"
#include "dectnrp/upper/p2p/procedure/steady_rd.hpp"

#define TFW_P2P_PT_AGC_ENABLED
#define TFW_P2P_PT_AGC_CHANGE_TIMED_OR_IMMEDIATE_PT

namespace dectnrp::upper::tfw::p2p {

class steady_pt_t final : public steady_rd_t {
    public:
        explicit steady_pt_t(args_t& args, pt_t& pt_);
        ~steady_pt_t() = default;

        steady_pt_t() = delete;
        steady_pt_t(const steady_pt_t&) = delete;
        steady_pt_t& operator=(const steady_pt_t&) = delete;
        steady_pt_t(steady_pt_t&&) = delete;
        steady_pt_t& operator=(steady_pt_t&&) = delete;

        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override final;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        // work_pcc(), work_pdc() and work_pdc_error() are implemented in deriving classes
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override final;
        phy::machigh_phy_tx_t work_chscan(const phy::chscan_t& chscan) override final;

        phy::irregular_report_t entry() override final;

    private:
        pt_t& pt;

        // ##################################################
        // Radio Layer + PHY
        // -

        // ##################################################
        // MAC Layer

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
        // -

        // ##################################################
        // logging

        void worksub_callback_log(const int64_t now_64) const override final;
};

}  // namespace dectnrp::upper::tfw::p2p
