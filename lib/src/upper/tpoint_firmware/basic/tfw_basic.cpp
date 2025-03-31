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

#include "dectnrp/upper/tpoint_firmware/basic/tfw_basic.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"

namespace dectnrp::upper::tfw::basic {

const std::string tfw_basic_t::firmware_name("basic");

tfw_basic_t::tfw_basic_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {}

void tfw_basic_t::work_start_imminent(const int64_t start_time_64) {
    dectnrp_assert(0 <= start_time_64, "start_time_64 too small");

    // value has no other function than being used in dectnrp_assert()
    barrier_time_64 = start_time_64;
}

phy::machigh_phy_t tfw_basic_t::work_regular(const phy::phy_mac_reg_t& phy_mac_reg) {
    dectnrp_assert(barrier_time_64 < phy_mac_reg.time_report.chunk_time_end_64,
                   "chunk_time_end_64 not strictly increasing");

    dectnrp_assert(barrier_time_64 < phy_mac_reg.time_report.barrier_time_64,
                   "barrier_time_64 not strictly increasing");

    // values have no other function than being used in dectnrp_assert()
    barrier_time_64 = phy_mac_reg.time_report.barrier_time_64;
    sync_time_last_64 = phy_mac_reg.time_report.sync_time_last_64;

    return phy::machigh_phy_t();
}

phy::maclow_phy_t tfw_basic_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    dectnrp_assert(sync_time_last_64 < phy_maclow.sync_report.fine_peak_time_64,
                   "sync_time_last_64 out-of-order");

    dectnrp_assert(barrier_time_64 <= phy_maclow.sync_report.fine_peak_time_64,
                   "fine_peak_time_64 before barrier_time_64");

    return phy::maclow_phy_t();
}

phy::machigh_phy_t tfw_basic_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_basic_t::work_upper(const upper::upper_report_t& upper_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_basic_t::work_chscan_async(const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

std::vector<std::string> tfw_basic_t::start_threads() {
    dectnrp_log_inf("start() called, press ctrl+c to stop program");
    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

std::vector<std::string> tfw_basic_t::stop_threads() {
    dectnrp_log_inf("stop() called");
    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

}  // namespace dectnrp::upper::tfw::basic
