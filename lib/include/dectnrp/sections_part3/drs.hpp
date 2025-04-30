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

#include "dectnrp/common/complex.hpp"
#include "dectnrp/common/multidim.hpp"
#include "dectnrp/sections_part3/df.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::section3 {

class drs_t final : public df_t {
    public:
        explicit drs_t(const uint32_t b_max = 1, const uint32_t N_TS_max = 1);

        void set_configuration(const uint32_t b, const uint32_t N_TS) override final;
        bool is_symbol_index(const uint32_t l) override final;

        /// get indices of DRS cells
        const common::vec2d<uint32_t>& get_k_i();

        /// get values of DRS cells
        const common::vec2d<cf_t>& get_y_DRS_i();

        /**
         * \brief The return value of either 0 or 4 represents the first transmit stream the values
         * of y_DRS_i have to be written to at indices k_i.
         *
         * \return 0 or 4
         */
        uint32_t get_ts_idx_first();

        /**
         * \brief The return value of either 0, 1, 3 or 7 represents the last transmit stream index
         * the values of y_DRS_i have to be written to at indices k_i. After utilizing k_i and
         * y_DRS_i for the current symbol, this function advances internal values for the next DRS
         * symbol.
         *
         * \return 0, 1, 3 or 7
         */
        uint32_t get_ts_idx_last_and_advance();

        /// convenience function to determine linear indices of PCC
        static common::vec2d<uint32_t> get_k_i_linear(const uint32_t u,
                                                      const uint32_t b,
                                                      const uint32_t N_PACKET_symb,
                                                      const uint32_t N_TS);

        static uint32_t get_N_step(const uint32_t N_TS_or_N_eff_TX);
        static uint32_t get_nof_drs_subc(const uint32_t b);

    private:
        /**
         * \brief See Figure 4.5-2 and Figure 4.5-3.
         *
         * For symbol index 1 and 2, the indices k_i of transmit stream 0/4, 1/5, 2/5 and 3/7 are
         * the same. For symbol index 6 and 11/12, the indices k_i of transmit stream 2/6, 3/7, 0/4
         * and 1/5 are also the same.
         *
         * Each vector will have up to 6 rows, each containing one matrix. Six values of beta and up
         * to four transmit streams.
         *
         * row 0:   [4][14]     // 56/4
         * row 1:   [4][28]     // 112/4
         * row 2:   [4][56]     // 224/4
         * row 3:   [4][112]    // 448/4
         * row 4:   [4][168]    // 672/4
         * row 5:   [4][224]    // 896/4
         */
        common::vec3d<uint32_t> k_i_for_1_2;
        common::vec3d<uint32_t> k_i_for_6_or_11_12;

        /**
         * \brief See Figure 4.5-2 and Figure 4.5-3
         *
         * Each vector will have up to 6 rows, each containing one matrix. Six values of beta and up
         * to four transmit streams.
         *
         * row 0:   [4][14]     // 56/4
         * row 1:   [4][28]     // 112/4
         * row 2:   [4][56]     // 224/4
         * row 3:   [4][112]    // 448/4
         * row 4:   [4][168]    // 672/4
         * row 5:   [4][224]    // 896/4
         */
        common::vec3d<cf_t> y_DRS_i_TS_0_to_3;
        common::vec3d<cf_t> y_DRS_i_TS_4_to_7;

        // these variables are set for each packet
        uint32_t config_b_idx;
        uint32_t config_N_TS;

        /// index of next symbol containing DRS
        uint32_t l_next;

        common::vec2d<uint32_t>* k_i_effective;
        common::vec2d<cf_t>* y_DRS_i_effective;
        uint32_t TS_idx_first_effective;

        void switch_k_i_pointer();
        void switch_y_DRS_i_pointer();

        static constexpr cf_t y_b_1[56] = {
            {1, 0},  {1, 0},  {1, 0},  {1, 0},  {-1, 0}, {1, 0},  {1, 0},  {-1, 0}, {-1, 0},
            {1, 0},  {1, 0},  {1, 0},  {1, 0},  {-1, 0}, {1, 0},  {-1, 0}, {1, 0},  {1, 0},
            {-1, 0}, {1, 0},  {-1, 0}, {1, 0},  {-1, 0}, {1, 0},  {1, 0},  {1, 0},  {1, 0},
            {1, 0},  {-1, 0}, {1, 0},

            {-1, 0}, {-1, 0}, {1, 0},  {1, 0},  {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {1, 0},
            {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {1, 0},  {1, 0},  {1, 0},  {-1, 0},
            {1, 0},  {1, 0},  {-1, 0}, {-1, 0}, {1, 0},  {-1, 0}, {-1, 0}, {-1, 0}};

        static std::vector<uint32_t> get_k_i_single(const phyres::k_b_OCC_vec_t& k_b_OCC_vec,
                                                    const uint32_t b_idx,
                                                    const uint32_t t,
                                                    const uint32_t n);

        static std::vector<cf_t> get_Y_DRS_i_single(const uint32_t b_idx, const uint32_t t);
};

}  // namespace dectnrp::section3
