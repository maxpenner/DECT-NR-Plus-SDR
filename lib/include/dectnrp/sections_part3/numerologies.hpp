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

#pragma once

#include <cstdint>
#include <cstdio>

namespace dectnrp::sp3 {

struct numerologies_t {
        uint32_t u;
        uint32_t b;

        uint32_t delta_u_f;
        double T_u_symb;
        uint32_t N_SLOT_u_symb;
        uint32_t N_SLOT_u_subslot;
        // double GI_u;

        uint32_t B_u_b_DFT;
        // double T_u_b_s;
        uint32_t N_b_DFT;
        uint32_t N_b_CP;
        uint32_t N_b_OCC;
        // uint32_t B_u_b_TX;

        // also calculate the guards
        uint32_t N_guards_top;
        uint32_t N_guards_bottom;
};

numerologies_t get_numerologies(const uint32_t u, const uint32_t b);

}  // namespace dectnrp::sp3
