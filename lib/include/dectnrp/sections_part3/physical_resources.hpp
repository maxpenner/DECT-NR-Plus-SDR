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

#include <array>
#include <cstdint>

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::sp3::phyres {

// clang-format off
static constexpr std::array<uint32_t, 17> b2b_idx{0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 5};
static constexpr std::array<uint32_t, 6> b_idx2b{1, 2, 4, 8, 12, 16};
static constexpr std::array<uint32_t, 6> N_b_DFT_lut{64, 128, 256, 512, 768, 1024};
static constexpr std::array<uint32_t, 6> N_b_CP_lut{8, 16, 32, 64, 96, 128};
static constexpr std::array<uint32_t, 6> N_b_OCC_lut{56, 112, 224, 448, 672, 896};
static constexpr std::array<uint32_t, 6> N_b_OCC_plus_DC_lut{57, 113, 225, 449, 673, 897};
static constexpr std::array<uint32_t, 6> guards_top_lut{3, 7, 15, 31, 47, 63};
static constexpr std::array<uint32_t, 6> guards_bottom_lut{4, 8, 16, 32, 48, 64};
// clang-format on

/// 1->0, 2->1, 4->2, 8->3
static constexpr uint32_t N_TS_2_N_TS_idx_vec[9] = {0, 0, 1, 0, 2, 0, 0, 0, 3};
static constexpr uint32_t N_TS_idx_2_N_TS_vec[4] = {1, 2, 4, 8};
static constexpr uint32_t N_TS_2_nof_STF_templates_vec[9] = {0, 1, 2, 0, 3, 0, 0, 0, 4};

struct k_b_OCC_t {
        int32_t b_1[56];
        int32_t b_2[112];
        int32_t b_4[224];
        int32_t b_8[448];
        int32_t b_12[672];
        int32_t b_16[896];
};

struct k_b_OCC_vec_t {
        /**
         * \brief vector b_x will have 6 rows
         *
         * row 0:   56 elements  (b=1)
         * row 1:   112 elements (b=2)
         * row 2:   224 elements (b=4)
         * row 3:   448 elements (b=8)
         * row 4:   672 elements (b=12)
         * row 5:   896 elements (b=16)
         */
        common::vec2d<int32_t> b_x;
};

k_b_OCC_t get_k_b_OCC();
k_b_OCC_vec_t get_k_b_OCC_vec();

}  // namespace dectnrp::sp3::phyres
