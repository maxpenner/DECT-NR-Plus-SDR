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

#include "dectnrp/phy/rx/sync/irregular_report.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

irregular_report_t irregular_report_t::get_same_with_time_increment(
    const int64_t time_increment_64) const {
    dectnrp_assert(has_finite_time(), "has no finite time");
    dectnrp_assert(0 < time_increment_64, "increment must be positive");
    dectnrp_assert(time_increment_64 < undefined_late, "increment must be finite");

    return irregular_report_t(call_asap_after_this_time_has_passed_64 + time_increment_64, handle);
};

}  // namespace dectnrp::phy
