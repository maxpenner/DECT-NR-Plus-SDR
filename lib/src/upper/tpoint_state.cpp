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

#include "dectnrp/upper/tpoint_state.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::upper {

phy::irregular_report_t tpoint_state_t::work_start([[maybe_unused]] const int64_t start_time_64) {
    dectnrp_assert_failure("uncallable");
    return phy::irregular_report_t();
};

void tpoint_state_t::work_stop() { dectnrp_assert_failure("uncallable"); };

}  // namespace dectnrp::upper
