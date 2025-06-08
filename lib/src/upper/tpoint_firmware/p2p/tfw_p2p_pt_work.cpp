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

#include "dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_pt.hpp"
//

#include <functional>

namespace dectnrp::upper::tfw::p2p {

phy::irregular_report_t tfw_p2p_pt_t::work_start_imminent(const int64_t start_time_64) {
    // what is the next full second after PHY becomes operational?
    const int64_t A = duration_lut.get_N_samples_at_next_full_second(start_time_64);

    // initialize regular callback for prints
    callbacks.add_callback(
        std::bind(&tfw_p2p_pt_t::worksub_callback_log, this, std::placeholders::_1),
        A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001),
        duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001,
                                                 worksub_callback_log_period_sec));

    return phy::irregular_report_t(start_time_64 + allocation_ft.get_beacon_period());
}

phy::machigh_phy_t tfw_p2p_pt_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_p2p_pt_t::work_irregular(const phy::irregular_report_t& irregular_report) {
    // update time of callbacks
    callbacks.run(buffer_rx.get_rx_time_passed());

    phy::machigh_phy_t ret;

    ret.irregular_report =
        irregular_report.get_with_time_increment(allocation_ft.get_beacon_period());

    return ret;
}

phy::machigh_phy_t tfw_p2p_pt_t::work_application(
    [[maybe_unused]] const upper::upper_report_t& upper_report) {
    phy::machigh_phy_t machigh_phy;

    worksub_tx_unicast_consecutive(machigh_phy);

    return machigh_phy;
}

phy::machigh_phy_tx_t tfw_p2p_pt_t::work_chscan_async(
    [[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

}  // namespace dectnrp::upper::tfw::p2p
