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

#include "dectnrp/upper/p2p/procedure/dissociation_ft.hpp"
#include "dectnrp/upper/p2p/procedure/nop.hpp"
#include "dectnrp/upper/p2p/procedure/resource.hpp"
#include "dectnrp/upper/p2p/procedure/steady_ft.hpp"
#include "dectnrp/upper/p2p/tfw_p2p_rd.hpp"

namespace dectnrp::upper::tfw::p2p {

class tfw_p2p_ft_t final : public tfw_p2p_rd_t {
    public:
        explicit tfw_p2p_ft_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_p2p_ft_t() = default;

        tfw_p2p_ft_t() = delete;
        tfw_p2p_ft_t(const tfw_p2p_ft_t&) = delete;
        tfw_p2p_ft_t& operator=(const tfw_p2p_ft_t&) = delete;
        tfw_p2p_ft_t(tfw_p2p_ft_t&&) = delete;
        tfw_p2p_ft_t& operator=(tfw_p2p_ft_t&&) = delete;

        static const std::string firmware_name;

        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override final;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override final;
        phy::machigh_phy_t work_pdc(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t work_pdc_error(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override final;
        phy::machigh_phy_tx_t work_channel(const phy::chscan_t& chscan) override final;

    private:
        void init_radio() override final;
        void init_simulation_if_detected() override final;
        void init_appiface() override final;
        [[nodiscard]] phy::irregular_report_t state_transitions() override final;

        /// data of fixed termination point shared with all states
        ft_t ft;

        /// states
        std::unique_ptr<resource_t> resource;
        std::unique_ptr<steady_ft_t> steady_ft;
        std::unique_ptr<dissociation_ft_t> dissociation_ft;
        std::unique_ptr<nop_t> nop;
};

}  // namespace dectnrp::upper::tfw::p2p
