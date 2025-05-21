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

#include "dectnrp/sections_part3/derivative/duration_ec.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp3 {

duration_ec_t get_duration_ec_depending_on_mu(const uint32_t u) {
    dectnrp_assert(std::has_single_bit(u) && u <= 8, "u undefined");

    switch (u) {
        case 1:
            return duration_ec_t::subslot_u1_001;
        case 2:
            return duration_ec_t::subslot_u2_001;
        case 4:
            return duration_ec_t::subslot_u4_001;
    }

    return duration_ec_t::subslot_u8_001;
}

}  // namespace dectnrp::sp3
