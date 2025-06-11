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

#include "dectnrp/upper/p2p/procedure/steady_ft.hpp"
//

#include <functional>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::upper::tfw::p2p {

phy::irregular_report_t steady_ft_t::work_start_imminent(const int64_t start_time_64) {
    // what is the next full second after start_time_64?
    const int64_t A = duration_lut.get_N_samples_at_next_full_second(start_time_64);

#ifdef TFW_P2P_FT_ALIGN_BEACON_START_TO_FULL_SECOND_OR_CORRECT_OFFSET
    // time first beacon is transmitted
    const int64_t B = A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001);
#else
    // time first beacon is transmitted
    const int64_t B = A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001) +
                      hw.get_pps_to_full_second_measured_samples();
#endif

    // set first beacon transmission time, beacon is aligned with full second
    rd.allocation_ft.set_beacon_time_scheduled(B);

#ifdef TFW_P2P_EXPORT_PPX
    dectnrp_assert(B - duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001) > 0,
                   "time out-of-order");

    // set virtual time of first rising edge, next edge is then aligned with the first beacon
    rd.ppx.set_ppx_rising_edge(B -
                               duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001));
#endif

    // initialize regular callback for logs
    rd.callbacks.add_callback(
        std::bind(&steady_ft_t::worksub_callback_log, this, std::placeholders::_1),
        A + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 500),
        duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::s001,
                                                 rd.worksub_callback_log_period_sec));

#ifdef TFW_P2P_EXPORT_PPX
    rd.callbacks.add_callback(std::bind(&steady_ft_t::worksub_callback_ppx,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3),
                              B - rd.ppx.get_ppx_time_advance_samples(),
                              rd.ppx.get_ppx_period_warped());
#endif

    return phy::irregular_report_t(
        rd.allocation_ft.get_beacon_time_scheduled_minus_prepare_duration());
}

phy::machigh_phy_t steady_ft_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t steady_ft_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    phy::machigh_phy_t ret;

    dectnrp_assert(irregular_report.call_asap_after_this_time_has_passed_64 <
                       rd.allocation_ft.get_beacon_time_scheduled(),
                   "too late");

    dectnrp_assert(0 < irregular_report.get_recognition_delay(), "time out-of-order");

    /*
    dectnrp_assert(irregular_report.get_recognition_delay() <
                       duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::slot001, 1),
                   "too late");
     */

    // try defining beacon transmission
    if (!worksub_tx_beacon(ret)) {
        dectnrp_assert_failure("beacon not transmitted");
    }

    worksub_tx_unicast_consecutive(ret);

    ret.irregular_report = phy::irregular_report_t(
        rd.allocation_ft.get_beacon_time_scheduled_minus_prepare_duration());

    rd.callbacks.run(buffer_rx.get_rx_time_passed());

    return ret;
}

phy::machigh_phy_t steady_ft_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    phy::machigh_phy_t machigh_phy;

    worksub_tx_unicast_consecutive(machigh_phy);

    return machigh_phy;
}

phy::machigh_phy_tx_t steady_ft_t::work_chscan_async([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

}  // namespace dectnrp::upper::tfw::p2p
