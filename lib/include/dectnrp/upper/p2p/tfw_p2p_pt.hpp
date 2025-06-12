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

#include <memory>

#include "dectnrp/upper/p2p/procedure/association.hpp"
#include "dectnrp/upper/p2p/procedure/deassociation.hpp"
#include "dectnrp/upper/p2p/procedure/nop.hpp"
#include "dectnrp/upper/p2p/procedure/steady_pt.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::p2p {

class tfw_p2p_pt_t final : public tpoint_t {
    public:
        explicit tfw_p2p_pt_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_p2p_pt_t() = default;

        tfw_p2p_pt_t() = delete;
        tfw_p2p_pt_t(const tfw_p2p_pt_t&) = delete;
        tfw_p2p_pt_t& operator=(const tfw_p2p_pt_t&) = delete;
        tfw_p2p_pt_t(tfw_p2p_pt_t&&) = delete;
        tfw_p2p_pt_t& operator=(tfw_p2p_pt_t&&) = delete;

        static const std::string firmware_name;

        phy::irregular_report_t work_start(const int64_t start_time_64) override final;
        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override final;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override final;
        phy::machigh_phy_t work_pdc_async(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override final;
        phy::machigh_phy_tx_t work_chscan_async(const phy::chscan_t& chscan) override final;
        void work_stop() override final;

    private:
        /// called once the active state requests a state change
        void state_transitions();

        /// data of radio device shared with all states
        rd_t rd;

        /// data of portable termination point shared with all states
        pt_t pt;

        /// states
        std::unique_ptr<association_t> association;
        std::unique_ptr<steady_pt_t> steady_pt;
        std::unique_ptr<deassociation_t> deassociation;
        std::unique_ptr<nop_t> nop;

        /// pointer to currently active state
        tpoint_t* state{nullptr};
};

}  // namespace dectnrp::upper::tfw::p2p
