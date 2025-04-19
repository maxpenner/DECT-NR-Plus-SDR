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

#include "dectnrp/mac/feedback_plan.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::mac {

feedback_plan_t::feedback_plan_t(const std::vector<uint32_t>&& feedback_format_vec_)
    : feedback_format_vec(feedback_format_vec_),
      idx(0) {}

uint32_t feedback_plan_t::get_current_feedback_format() { return feedback_format_vec.at(idx); }

void feedback_plan_t::set_next_feedback_format() {
    ++idx;
    idx %= feedback_format_vec.size();
}

}  // namespace dectnrp::mac
