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

#include "dectnrp/common/multidim.hpp"
#include "dectnrp/sections_part3/df.hpp"

namespace dectnrp::sp3 {

class pdc_t final : public df_pxc_t {
    public:
        explicit pdc_t(const uint32_t b_max = 1, const uint32_t N_TS_max = 1);

        void set_configuration(const uint32_t b, const uint32_t N_TS) override final;
        bool is_symbol_index(const uint32_t l) override final;
        const std::vector<uint32_t>& get_k_i_one_symbol() const override final;

        static uint32_t get_N_DF_symb(const uint32_t N_PACKET_symb, const uint32_t u);

        static uint32_t get_nof_OFDM_symbols_carrying_DRS_per_TS(const uint32_t u,
                                                                 const uint32_t N_PACKET_symb,
                                                                 const uint32_t N_eff_TX);
        static uint32_t get_N_DRS_subc(const uint32_t u,
                                       const uint32_t N_PACKET_symb,
                                       const uint32_t N_eff_TX,
                                       const uint32_t N_b_OCC);

        static uint32_t get_N_PDC_subc(const uint32_t N_PACKET_symb,
                                       const uint32_t u,
                                       const uint32_t N_eff_TX,
                                       const uint32_t N_b_OCC);

    private:
        uint32_t config_b_idx;
        uint32_t config_N_TS;
        uint32_t config_N_TS_idx;

        common::vec2d<uint32_t>* k_i_all_symbols_effective;

        uint32_t l_limit;
        uint32_t l_repeat;
        uint32_t l_equivalent;

        /// convenience function to determine linear indices of PDC
        static std::vector<uint32_t> get_k_i_linear(const uint32_t b, const uint32_t N_TS);

        static std::vector<uint32_t> idx2sub_col_zero_vec(const std::vector<uint32_t> linear_idx,
                                                          const uint32_t dim_column);

        static common::vec2d<uint32_t> idx2sub_row_zero_vec(const std::vector<uint32_t> linear_idx,
                                                            const uint32_t dim_column,
                                                            const uint32_t offset);
};

}  // namespace dectnrp::sp3
