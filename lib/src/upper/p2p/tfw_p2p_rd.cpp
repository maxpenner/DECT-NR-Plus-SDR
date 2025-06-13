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

#include "dectnrp/upper/p2p/tfw_p2p_rd.hpp"

namespace dectnrp::upper::tfw::p2p {

phy::irregular_report_t tfw_p2p_rd_t::work_start(const int64_t start_time_64) {
    [[maybe_unused]] const auto irregular_report = tpoint_state->entry();

    dectnrp_assert(!irregular_report.has_finite_time(),
                   "entry should return empty irregular_report_t");

    return tpoint_state->work_start(start_time_64);
}

}  // namespace dectnrp::upper::tfw::p2p
