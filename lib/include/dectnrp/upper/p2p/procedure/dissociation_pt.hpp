/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/upper/p2p/data/pt.hpp"
#include "dectnrp/upper/p2p/data/rd.hpp"
#include "dectnrp/upper/p2p/procedure/args.hpp"
#include "dectnrp/upper/tpoint_state.hpp"

namespace dectnrp::upper::tfw::p2p {

class dissociation_pt_t final : public tpoint_state_t {
    public:
        explicit dissociation_pt_t(args_t& args, pt_t& pt_);
        ~dissociation_pt_t() = default;

        dissociation_pt_t() = delete;
        dissociation_pt_t(const dissociation_pt_t&) = delete;
        dissociation_pt_t& operator=(const dissociation_pt_t&) = delete;
        dissociation_pt_t(dissociation_pt_t&&) = delete;
        dissociation_pt_t& operator=(dissociation_pt_t&&) = delete;

        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override final;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override final;
        phy::machigh_phy_t work_pdc(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t work_pdc_error(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override final;
        phy::machigh_phy_tx_t work_channel(const phy::chscan_t& chscan) override final;

        phy::irregular_report_t entry() override final;

    private:
        [[maybe_unused]] rd_t& rd;
        [[maybe_unused]] pt_t& pt;
};

}  // namespace dectnrp::upper::tfw::p2p
