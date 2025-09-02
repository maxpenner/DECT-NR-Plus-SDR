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

#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::sp3::phyres {

void fill_subc(int* out, const int N_b_OCC) {
    uint32_t idx = 0;

    for (int i = -N_b_OCC / 2; i <= -1; ++i) {
        out[idx++] = i;
    }

    for (int i = 1; i <= N_b_OCC / 2; ++i) {
        out[idx++] = i;
    }
}

k_b_OCC_t get_k_b_OCC() {
    k_b_OCC_t k_b_OCC;

    fill_subc(k_b_OCC.b_1, N_b_OCC_lut[0]);
    fill_subc(k_b_OCC.b_2, N_b_OCC_lut[1]);
    fill_subc(k_b_OCC.b_4, N_b_OCC_lut[2]);
    fill_subc(k_b_OCC.b_8, N_b_OCC_lut[3]);
    fill_subc(k_b_OCC.b_12, N_b_OCC_lut[4]);
    fill_subc(k_b_OCC.b_16, N_b_OCC_lut[5]);

    return k_b_OCC;
}

k_b_OCC_vec_t get_k_b_OCC_vec() {
    k_b_OCC_vec_t k_b_OCC_vec;

    for (uint32_t b_idx = 0; b_idx < 6; ++b_idx) {
        // add new row
        k_b_OCC_vec.b_x.push_back(std::vector<int>());

        int N_b_OCC = N_b_OCC_lut[b_idx];

        for (int i = -N_b_OCC / 2; i <= -1; ++i) {
            k_b_OCC_vec.b_x[b_idx].push_back(i);
        }

        for (int i = 1; i <= N_b_OCC / 2; ++i) {
            k_b_OCC_vec.b_x[b_idx].push_back(i);
        }
    }

    return k_b_OCC_vec;
}

}  // namespace dectnrp::sp3::phyres
