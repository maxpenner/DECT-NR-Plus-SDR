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

#pragma once

#include <complex>
#include <cstdint>
#include <vector>

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::section3 {

class Y_i_t {
    public:
        Y_i_t(const uint32_t N_b_OCC_max, const uint32_t N_SS_max);
        ~Y_i_t();

        Y_i_t() = delete;
        Y_i_t(const Y_i_t&) = delete;
        Y_i_t& operator=(const Y_i_t&) = delete;
        Y_i_t(Y_i_t&&) = delete;
        Y_i_t& operator=(Y_i_t&&) = delete;

        uint32_t get_modulo(const uint32_t N_TS) const;

        const common::vec2d<uint32_t>& get_index_mat_N_TS_x(const uint32_t N_TS) const;

        cf_t* get_pattern() const { return pattern_minus_1_j_1_j; }

    private:
        cf_t* pattern_minus_1_j_1_j{nullptr};

        /**
         * \brief vector W will have 3 rows for 3 different values of N_TS = 2, 4 or 8
         *
         * row 0:   [ 1][ 2]
         * row 1:   [ 6][ 2]
         * row 2:   [12][ 2]
         */
        common::vec3d<uint32_t> index_N_TS_x;

        static constexpr uint32_t N_TS248_idx_vec[9] = {0, 0, 0, 0, 1, 0, 0, 0, 2};
};

}  // namespace dectnrp::section3
