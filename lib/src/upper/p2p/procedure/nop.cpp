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

#include "dectnrp/upper/p2p/procedure/nop.hpp"

namespace dectnrp::upper::tfw::p2p {

nop_t::nop_t(args_t& args)
    : tpoint_state_t(args.tpoint_config, args.mac_lower, args.leave_callback) {}

phy::irregular_report_t nop_t::work_start([[maybe_unused]] const int64_t start_time_64) {
    return phy::irregular_report_t();
}

phy::machigh_phy_t nop_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t nop_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    return phy::machigh_phy_t();
}

phy::maclow_phy_t nop_t::work_pcc([[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::machigh_phy_t nop_t::work_pdc_async([[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t nop_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t nop_t::work_chscan_async([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

void nop_t::work_stop() {}

void nop_t::entry() {};

}  // namespace dectnrp::upper::tfw::p2p
