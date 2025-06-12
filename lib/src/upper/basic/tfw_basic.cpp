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

#include "dectnrp/upper/basic/tfw_basic.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"

namespace dectnrp::upper::tfw::basic {

const std::string tfw_basic_t::firmware_name("basic");

tfw_basic_t::tfw_basic_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {}

phy::irregular_report_t tfw_basic_t::work_start(const int64_t start_time_64) {
    dectnrp_assert(0 <= start_time_64, "start_time_64 too small");

    // value has no other function than being used in dectnrp_assert()
    barrier_time_64 = start_time_64;

    return phy::irregular_report_t();
}

phy::machigh_phy_t tfw_basic_t::work_regular(const phy::regular_report_t& regular_report) {
    dectnrp_assert(barrier_time_64 < regular_report.chunk_time_end_64,
                   "chunk_time_end_64 not strictly increasing");

    dectnrp_assert(barrier_time_64 < regular_report.barrier_time_64,
                   "barrier_time_64 not strictly increasing");

    // values have no other function than being used in dectnrp_assert()
    barrier_time_64 = regular_report.barrier_time_64;
    sync_time_last_64 = regular_report.sync_time_last_64;

    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_basic_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    return phy::machigh_phy_t();
}

phy::maclow_phy_t tfw_basic_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    dectnrp_assert(sync_time_last_64 < phy_maclow.sync_report.fine_peak_time_64,
                   "sync_time_last_64 out-of-order");

    dectnrp_assert(barrier_time_64 <= phy_maclow.sync_report.fine_peak_time_64,
                   "fine_peak_time_64 before barrier_time_64");

    return phy::maclow_phy_t();
}

phy::machigh_phy_t tfw_basic_t::work_pdc_async(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_basic_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_basic_t::work_chscan_async([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

void tfw_basic_t::work_stop() { dectnrp_log_inf("work_stop() called"); }

}  // namespace dectnrp::upper::tfw::basic
