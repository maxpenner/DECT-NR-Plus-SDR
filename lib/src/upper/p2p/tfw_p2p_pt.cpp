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

#include "dectnrp/upper/p2p/tfw_p2p_pt.hpp"

namespace dectnrp::upper::tfw::p2p {

const std::string tfw_p2p_pt_t::firmware_name("p2p_pt");

tfw_p2p_pt_t::tfw_p2p_pt_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    // same callback for every state
    state_t::leave_callback_t leave_callback = std::bind(&tfw_p2p_pt_t::state_transitions, this);

    // common arguments of all states
    args_t args{.tpoint_config = tpoint_config_,
                .mac_lower = mac_lower_,
                .leave_callback = leave_callback,
                .rd = rd};

    association = std::make_unique<tfw::p2p::association_t>(args, pt);

    steady_pt = std::make_unique<tfw::p2p::steady_pt_t>(args, pt);

    deassociation = std::make_unique<tfw::p2p::deassociation_t>(args, pt);

    nop = std::make_unique<tfw::p2p::nop_t>(args);

    // set first state
    state = steady_pt.get();
}

phy::irregular_report_t tfw_p2p_pt_t::work_start(const int64_t start_time_64) {
    return state->work_start(start_time_64);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_regular(const phy::regular_report_t& regular_report) {
    return state->work_regular(regular_report);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_irregular(const phy::irregular_report_t& irregular_report) {
    return state->work_irregular(irregular_report);
}

phy::maclow_phy_t tfw_p2p_pt_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    return state->work_pcc(phy_maclow);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    return state->work_pdc_async(phy_machigh);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_application(
    const application::application_report_t& application_report) {
    return state->work_application(application_report);
}

phy::machigh_phy_tx_t tfw_p2p_pt_t::work_chscan_async(const phy::chscan_t& chscan) {
    return state->work_chscan_async(chscan);
}

void tfw_p2p_pt_t::work_stop() { state->work_stop(); }

void tfw_p2p_pt_t::state_transitions() {}

}  // namespace dectnrp::upper::tfw::p2p
