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

#include "dectnrp/sections_part3/numerologies.hpp"

#include <bit>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

namespace dectnrp::sp3 {

numerologies_t get_numerologies(const uint32_t u, const uint32_t b) {
    dectnrp_assert(std::has_single_bit(u) and u <= 8, "u undefined");
    dectnrp_assert((std::has_single_bit(b) && b <= 16) || b == 12, "b undefined");

    numerologies_t q;

    q.u = u;
    q.b = b;

    q.delta_u_f = u * constants::subcarrier_spacing_min_u_b;
    q.T_u_symb = (64.0 + 8.0) / 64.0 / static_cast<double>(q.delta_u_f);
    q.N_SLOT_u_symb = u * 10;
    q.N_SLOT_u_subslot = u * 2;

    /*
    switch (u) {
        case 1:
            q.GI_u = 4.0 / 9.0 * (double)q.T_u_symb;  // Figure 5.1-1
            break;
        case 2:
        case 4:
            q.GI_u = (double)q.T_u_symb;  // Figure 5.1-2
            break;
        case 8:
            q.GI_u = 2.0 * (double)q.T_u_symb;  // Figure 5.1-3
            break;
    }
     */

    q.B_u_b_DFT = b * 64 * q.delta_u_f;
    // q.T_u_b_s = 1.0 / (double)q.B_u_b_DFT;
    q.N_b_DFT = b * 64;
    q.N_b_CP = b * 8;
    q.N_b_OCC = b * 56;
    // q.B_u_b_TX = (q.N_b_OCC + 1) * q.delta_u_f;  // +1 for DC

    q.N_guards_top = (q.N_b_DFT - q.N_b_OCC) / 2 - 1;
    q.N_guards_bottom = q.N_guards_top + 1;

    return q;
}

}  // namespace dectnrp::sp3
