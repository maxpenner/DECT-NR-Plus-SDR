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

#include <array>
#include <cstdint>
#include <vector>

#include "dectnrp/common/complex.hpp"
#include "dectnrp/common/multidim.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/sections_part3/stf_param.hpp"

namespace dectnrp::sp3 {

class stf_t {
    public:
        /**
         * \brief See 6.3.5 in ETSI TS 103 636-3: "Number of occupied subcarriers is four times
         * lower for STF symbol than for the other symbols." We increase the amplitude by a factor
         * of 2, this way the STF and all other symbols will have the same power in time domain.
         *
         * \param b_max
         * \param N_eff_TX_max
         * \param scale_
         */
        explicit stf_t(const uint32_t b_max = 1,
                       const uint32_t N_eff_TX_max = 1,
                       const float scale_ = 2.0f);

        const std::vector<cf_t>& get_stf(const uint32_t b, const uint32_t N_eff_TX) const;

        /**
         * \brief One slot always has a length of 10ms/24 = 416.6667us.
         *
         * STF lengths:
         *
         * u = 1 (7 repetitions):
         *      (1 + 3/4) * 64 * b = 112*b, 16*b per pattern
         *      14/9 * T_u_symb = 14/9 * 41.667us = 64.8148us
         *      14/9*1.125 = 3/4 + 1
         *
         *      subslot = 208.3333us
         *
         * u = 2 (9 repetitions):
         *      (1 + 5/4) * 64 * b = 144*b, 16*b per pattern
         *      2 * T_u_symb = 2 * 20.833us = 41.6667us
         *      2*1.125 = 1 + 5/4
         *
         *      subslot = 104.1667us
         *
         * u = 4 (9 repetitions):
         *      (1 + 5/4) * 64 * b = 144*b, 16*b per pattern
         *      2 * T_u_symb = 2 * 10.417us = 20.8333us
         *      2*1.125 = 1 + 5/4
         *
         *      subslot = 52.08333us
         *
         * u = 8 (9 repetitions):
         *      (1 + 5/4) * 64 * b = 144*b, 16*b per pattern
         *      2 * T_u_symb = 2 * 5.208us = 10.4167us
         *      2*1.125 = 1 + 5/4
         *
         *      subslot = 26.04167us
         *
         * Approximate length of coarse metric until peak without noise (from Matlab):
         *
         *      n_samples_STF * b * oversampling * (n_pattern-1) / n_pattern
         *
         * \param u
         * \return
         */
        static uint32_t get_N_samples_stf(const uint32_t u) {
            return u == 1 ? constants::N_samples_stf_u1 : constants::N_samples_stf_u248;
        }

        static uint32_t get_N_stf_pattern(const uint32_t u) {
            return u == 1 ? constants::N_stf_pattern_u1 : constants::N_stf_pattern_u248;
        }

        static uint32_t get_equivalent_u(const uint32_t N_stf_pattern) {
            return N_stf_pattern == constants::N_stf_pattern_u1 ? 1 : 2;
        }

        // ##################################################
        // the cover sequence introduced in V1.5.1

        static std::vector<float> get_cover_sequence(const uint32_t u);

        static void apply_cover_sequence(cf_t* dst,
                                         const cf_t* src,
                                         const uint32_t u,
                                         const uint32_t N_samples_stf_pattern_os);

        static void apply_cover_sequence(std::vector<cf_t*>& dst,
                                         const std::vector<cf_t*>& src,
                                         const uint32_t N_TX,
                                         const uint32_t u,
                                         const uint32_t N_samples_stf_pattern_os);

        static float get_cover_sequence_pairwise_product_single(const uint32_t i) {
            return cover_sequence[i] * cover_sequence[i + 1];
        }

        static std::vector<float> get_cover_sequence_pairwise_product(const uint32_t u);

    private:
        float scale;

        /**
         * \brief Vector W will have up to 6 rows, each containing one matrix. Six values of b and
         * up to 4 values of N_eff_TX. Any STF can be loaded with get_stf(), the DC carrier is
         * already inserted.
         *
         * row 0:   [4][56+1]
         * row 1:   [4][112+1]
         * row 2:   [4][224+1]
         * row 3:   [4][448+1]
         * row 4:   [4][672+1]
         * row 5:   [4][896+1]
         */
        common::vec3d<cf_t> y_STF_filled_b_x;

        static const std::vector<float> y_b_1;
        static const std::vector<float> y_b_2;
        static const std::vector<float> y_b_4;

        static constexpr std::array<float, constants::N_stf_pattern_u248> cover_sequence{
#ifdef SECTIONS_PART_3_STF_COVER_SEQUENCE_ACTIVE
            1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
#else
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
#endif

        static void fill_k_i(int* k_i, const int* k_b_OCC, const int N_b_OCC);

        static void fill_y_STF_i(cf_t* y_STF_i_b_x,
                                 const int N_b_OCC,
                                 const int N_eff_TX,
                                 const float scale);

        static std::vector<cf_t> fill_y_STF_filled(const cf_t* y_STF_i_b_x,
                                                   const int* k_i,
                                                   const int N_b_OCC);
};

}  // namespace dectnrp::sp3
