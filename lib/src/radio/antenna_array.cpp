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

#include "dectnrp/radio/antenna_array.hpp"

#include <set>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::radio {

antenna_array_t::antenna_array_t(const arrangement_t arrangement_, const float separation_)
    : arrangement(arrangement_),
      separation(std::vector<float>{separation_}) {
    dectnrp_assert(arrangement != arrangement_t::linear_uneven, "constructor for even separation");
};

antenna_array_t::antenna_array_t(const arrangement_t arrangement_,
                                 const std::vector<float> separation_)
    : arrangement(arrangement_),
      separation(separation_) {
    dectnrp_assert(arrangement == arrangement_t::linear_uneven,
                   "constructor for uneven separation");

    dectnrp_assert(std::set<double>(separation.begin(), separation.end()).size() > 1,
                   "constructor for uneven separation with even separation");
}

}  // namespace dectnrp::radio
