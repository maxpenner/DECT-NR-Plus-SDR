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

#include "dectnrp/upper/p2p/procedure/dissociation_pt.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::upper::tfw::p2p {

static constexpr uint32_t handle = 12345;

dissociation_pt_t::dissociation_pt_t(args_t& args, pt_t& pt_)
    : tpoint_state_t(args.tpoint_config, args.mac_lower, args.state_transitions_cb),
      rd(args.rd),
      pt(pt_) {}

phy::machigh_phy_t dissociation_pt_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t dissociation_pt_t::work_irregular(
    const phy::irregular_report_t& irregular_report) {
    phy::machigh_phy_t ret;

    if (irregular_report.handle == handle) {
        ret.irregular_report = state_transitions_cb();
    }

    return ret;
}

phy::maclow_phy_t dissociation_pt_t::work_pcc(
    [[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::machigh_phy_t dissociation_pt_t::work_pdc(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t dissociation_pt_t::work_pdc_error(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t dissociation_pt_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t dissociation_pt_t::work_channel(
    [[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

phy::irregular_report_t dissociation_pt_t::entry() {
    dectnrp_assert(rd.is_shutting_down(), "not shutting down");

    return phy::irregular_report_t(
        buffer_rx.get_rx_time_passed() +
            duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 100),
        handle);
}

}  // namespace dectnrp::upper::tfw::p2p
