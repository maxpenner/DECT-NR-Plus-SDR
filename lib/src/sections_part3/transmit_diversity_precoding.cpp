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

#include "dectnrp/sections_part3/transmit_diversity_precoding.hpp"

#include <algorithm>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

namespace dectnrp::sp3 {

Y_i_t::Y_i_t(const uint32_t N_b_OCC_max, const uint32_t N_SS_max) {
    const uint32_t min_len = std::max(constants::pcc_cells, N_b_OCC_max * N_SS_max);

    pattern_minus_1_j_1_j = srsran_vec_cf_malloc(min_len);

    for (uint32_t i = 0; i < min_len; i += 2) {
        pattern_minus_1_j_1_j[i] = cf_t{-1.0f, 1.0f};
        pattern_minus_1_j_1_j[i + 1] = cf_t{1.0f, -1.0f};
    }

    // initialize 3 rows
    for (uint32_t i = 0; i < 3; ++i) {
        index_N_TS_x.push_back(common::vec2d<uint32_t>());
    }

    // N_TS=2
    index_N_TS_x[0].push_back(std::vector<uint32_t>{0, 1});

    // N_TS=4
    index_N_TS_x[1].push_back(std::vector<uint32_t>{0, 1});
    index_N_TS_x[1].push_back(std::vector<uint32_t>{2, 3});
    index_N_TS_x[1].push_back(std::vector<uint32_t>{0, 2});
    index_N_TS_x[1].push_back(std::vector<uint32_t>{1, 3});
    index_N_TS_x[1].push_back(std::vector<uint32_t>{0, 3});
    index_N_TS_x[1].push_back(std::vector<uint32_t>{1, 2});

    // N_TS=12
    index_N_TS_x[2].push_back(std::vector<uint32_t>{0, 1});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{2, 3});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{4, 5});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{6, 7});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{0, 4});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{1, 5});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{2, 6});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{3, 7});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{0, 2});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{1, 3});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{4, 6});
    index_N_TS_x[2].push_back(std::vector<uint32_t>{5, 7});
}

Y_i_t::~Y_i_t() { free(pattern_minus_1_j_1_j); }

uint32_t Y_i_t::get_modulo(const uint32_t N_TS) const {
    switch (N_TS) {
        case 2:
            return 1;
        case 4:
            return 6;
    }

    // N_TS == 8
    return 12;
}

const common::vec2d<uint32_t>& Y_i_t::get_index_mat_N_TS_x(const uint32_t N_TS) const {
    dectnrp_assert(2 <= N_TS, "must be at least 2 transmit streams");

    return index_N_TS_x[N_TS248_idx_vec[N_TS]];
}

}  // namespace dectnrp::sp3
