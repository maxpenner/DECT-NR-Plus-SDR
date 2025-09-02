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

#include "dectnrp/radio/antenna_array.hpp"

#include <set>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::radio {

antenna_array_t::antenna_array_t(const arrangement_t arrangement_, const float spacing_)
    : arrangement(arrangement_),
      spacing(std::vector<float>{spacing_}) {
    dectnrp_assert(arrangement != arrangement_t::linear_uneven, "constructor for even spacing");
};

antenna_array_t::antenna_array_t(const arrangement_t arrangement_,
                                 const std::vector<float> spacing_)
    : arrangement(arrangement_),
      spacing(spacing_) {
    dectnrp_assert(arrangement == arrangement_t::linear_uneven, "constructor for uneven spacing");

    dectnrp_assert(std::set<double>(spacing.begin(), spacing.end()).size() > 1,
                   "constructor for uneven spacing with even spacing");
}

}  // namespace dectnrp::radio
