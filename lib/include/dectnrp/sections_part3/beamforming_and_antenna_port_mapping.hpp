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

#include <cstdint>
#include <vector>

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::section3 {

class W_t {
    public:
        W_t();

        const std::vector<cf_t>& get_W(const uint32_t N_TS,
                                       const uint32_t N_TX,
                                       const uint32_t codebook_idx) const;

        const common::vec2d<cf_t>& get_W(const uint32_t N_TS, const uint32_t N_TX) const;

        float get_scaling_factor(const uint32_t N_TS,
                                 const uint32_t N_TX,
                                 const uint32_t codebook_idx) const;

        const std::vector<float>& get_scaling_factor(const uint32_t N_TS,
                                                     const uint32_t N_TX) const;

        float get_scaling_factor_optimal_DAC(const uint32_t N_TS,
                                             const uint32_t N_TX,
                                             const uint32_t codebook_idx) const;

        const std::vector<float>& get_scaling_factor_optimal_DAC(const uint32_t N_TS,
                                                                 const uint32_t N_TX) const;

        static uint32_t get_codebook_index_max(const uint32_t N_TS, const uint32_t N_TX);

        static uint32_t get_codebook_index_nonzero(const uint32_t N_TS, const uint32_t N_TX);

    private:
        /*
         * Vector W will have 7 rows, each containing one matrix.
         * Should be called for N_TS=1 and N_TX=1 to get the correct scaling.
         *
         * row 0:   [ 1][ 1]    // [number of codebook indices][size of matrix W]
         * row 1:   [ 6][ 2]
         * row 2:   [28][ 4]
         * row 3:   [ 3][ 4]
         * row 4:   [22][ 8]
         * row 5:   [ 5][16]
         * row 6:   [ 1][64]
         */
        common::vec3d<cf_t> W;
        common::vec2d<float> scaling_factor;
        common::vec2d<float> scaling_factor_optimal_DAC;

        static constexpr int8_t j = 2;

        /// Table 6.3.4-1 to Table 6.3.4-6
        static const common::vec2d<int8_t> W_1, W_2, W_3, W_4, W_5, W_6;

        static constexpr common::array2d<uint8_t, 9, 9> N_TS_N_TX_idx = {
            {{0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 1, 0, 2, 0, 0, 0, 0},
             {0, 0, 3, 0, 4, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 5, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 6}}};

        static constexpr common::array2d<uint8_t, 9, 9> N_TS_N_TX_codebook_index_max = {
            {{0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 5, 0, 27, 0, 0, 0, 0},
             {0, 0, 2, 0, 21, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 4, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0}}};

        static constexpr common::array2d<uint8_t, 9, 9> N_TS_N_TX_codebook_index_nonzero = {
            {{0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 2, 0, 12, 0, 0, 0, 0},
             {0, 0, 1, 0, 14, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 3, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0},
             {0, 0, 0, 0, 0, 0, 0, 0, 0}}};

        static std::vector<cf_t> convert_to_cf32(const std::vector<int8_t> in);

        static float get_W_scaling_factor(const std::vector<int8_t> in);
};

}  // namespace dectnrp::section3
