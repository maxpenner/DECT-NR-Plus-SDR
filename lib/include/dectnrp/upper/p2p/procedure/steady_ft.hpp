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

#include "dectnrp/upper/p2p/data/ft.hpp"
#include "dectnrp/upper/p2p/procedure/args.hpp"
#include "dectnrp/upper/p2p/procedure/steady_rd.hpp"

// #define TFW_P2P_FT_ALIGN_BEACON_START_TO_FULL_SECOND_OR_CORRECT_OFFSET

namespace dectnrp::upper::tfw::p2p {

class steady_ft_t final : public steady_rd_t {
    public:
        explicit steady_ft_t(args_t& args, ft_t& ft_);
        ~steady_ft_t() = default;

        steady_ft_t() = delete;
        steady_ft_t(const steady_ft_t&) = delete;
        steady_ft_t& operator=(const steady_ft_t&) = delete;
        steady_ft_t(steady_ft_t&&) = delete;
        steady_ft_t& operator=(steady_ft_t&&) = delete;

        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override final;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        // work_pcc(), work_pdc() and work_pdc_error() are implemented in deriving classes
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override final;
        phy::machigh_phy_tx_t work_chscan(const phy::chscan_t& chscan) override final;

        phy::irregular_report_t entry() override final;

    private:
        ft_t& ft;

        // ##################################################
        // Radio Layer + PHY
        // -

        // ##################################################
        // MAC Layer

        // besides unicast for downlink, the FT also requires a beacon packet
        void init_packet_beacon();

        // clang-format off
        std::optional<phy::maclow_phy_t> worksub_pcc_10(const phy::phy_maclow_t& phy_maclow) override final;
        phy::maclow_phy_t worksub_pcc_20(const phy::phy_maclow_t& phy_maclow) override final;
        phy::maclow_phy_t worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) override final;
        
        phy::machigh_phy_t worksub_pdc_10(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t worksub_pdc_20(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) override final;
        // clang-format on

        bool worksub_tx_beacon(phy::machigh_phy_t& machigh_phy);

        void worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) override final;

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
