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

#include "dectnrp/sections_part2/reference_time.hpp"

namespace dectnrp::sp2 {

reference_time_accuracy_t get_reference_time_accuracy(const bool extreme_condition) {
    reference_time_accuracy_t ret;

    ret.extreme_condition = extreme_condition;

    if (extreme_condition) {
        ret.accuracy_ppm = 15;
    } else {
        ret.accuracy_ppm = 10;
    }

    return ret;
}

}  // namespace dectnrp::sp2
