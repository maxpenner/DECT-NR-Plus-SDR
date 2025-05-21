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

class pcc_t final : public df_pxc_t {
    public:
        explicit pcc_t(const uint32_t b_max = 1, const uint32_t N_TS_max = 1);

        void set_configuration(const uint32_t b, const uint32_t N_TS) override final;
        bool is_symbol_index(const uint32_t l) override final;
        const std::vector<uint32_t>& get_k_i_one_symbol() const override final;

        /// convenience function to determine linear indices of PCC
        static std::vector<uint32_t> get_k_i_linear(const uint32_t b, const uint32_t N_TS);

        /// at the receiver, we need to know the last OFDM symbol index with PCC subcarriers
        uint32_t get_pcc_symbol_idx_max() const { return ptr_l_all_symbols->back(); };

    private:
        uint32_t config_local_cnt;
        std::vector<uint32_t>* ptr_l_all_symbols;
        common::vec2d<uint32_t>* ptr_k_i_all_symbols;

        static std::vector<uint32_t> idx2sub_col(const std::vector<uint32_t> linear_idx,
                                                 uint32_t dim_column);

        static common::vec2d<uint32_t> idx2sub_row(const std::vector<uint32_t> linear_idx,
                                                   uint32_t dim_column,
                                                   uint32_t cnt_offset);
};

}  // namespace dectnrp::sp3
