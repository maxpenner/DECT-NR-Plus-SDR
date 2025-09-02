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

#include "dectnrp/sections_part3/mcs.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp3 {

mcs_t get_mcs(const uint32_t index) {
    dectnrp_assert(index <= 11, "MCS out of bound");

    mcs_t q;

    switch (q.index = index) {
        case 0:
            q.N_bps = 1;
            q.R_numerator = 1;
            q.R_denominator = 2;
            break;

        case 1:
            q.N_bps = 2;
            q.R_numerator = 1;
            q.R_denominator = 2;
            break;

        case 2:
            q.N_bps = 2;
            q.R_numerator = 3;
            q.R_denominator = 4;
            break;

        case 3:
            q.N_bps = 4;
            q.R_numerator = 1;
            q.R_denominator = 2;
            break;

        case 4:
            q.N_bps = 4;
            q.R_numerator = 3;
            q.R_denominator = 4;
            break;

        case 5:
            q.N_bps = 6;
            q.R_numerator = 2;
            q.R_denominator = 3;
            break;

        case 6:
            q.N_bps = 6;
            q.R_numerator = 3;
            q.R_denominator = 4;
            break;

        case 7:
            q.N_bps = 6;
            q.R_numerator = 5;
            q.R_denominator = 6;
            break;

        case 8:
            q.N_bps = 8;
            q.R_numerator = 3;
            q.R_denominator = 4;
            break;

        case 9:
            q.N_bps = 8;
            q.R_numerator = 5;
            q.R_denominator = 6;
            break;

        case 10:
            q.N_bps = 10;
            q.R_numerator = 3;
            q.R_denominator = 4;
            break;

        case 11:
            q.N_bps = 10;
            q.R_numerator = 5;
            q.R_denominator = 6;
            break;
    }

    return q;
}

}  // namespace dectnrp::sp3
