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

#include "dectnrp/sections_part3/pcc.hpp"

#include <algorithm>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/sections_part3/drs.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

// marks subcarriers as occupied, must be negative
static constexpr int32_t PCC_OCCUPIED_DC = -1;
static constexpr int32_t PCC_OCCUPIED_GUARDS = -2;
static constexpr int32_t PCC_OCCUPIED_DRS = -3;

namespace dectnrp::sp3 {

pcc_t::pcc_t(const uint32_t b_max, const uint32_t N_TS_max) {
    k_i_all_symbols.clear();

    const uint32_t b_idx_max = phyres::b2b_idx[b_max];

    // for each value of beta
    for (uint32_t b_idx = 0; b_idx <= b_idx_max; ++b_idx) {
        const uint32_t b = phyres::b_idx2b[b_idx];
        const uint32_t N_b_DFT = phyres::N_b_DFT_lut[b_idx];
        const uint32_t N_guards = phyres::guards_bottom_lut[b_idx];

        // add new rows
        l_all_symbols.push_back(common::vec2d<uint32_t>());
        k_i_all_symbols.push_back(common::vec3d<uint32_t>());

        // we have 1, 2, 4 or 8 transmit streams, each containing DRS subcarriers
        for (uint32_t N_TS = 1; N_TS <= N_TS_max; N_TS = N_TS * 2) {
            // determine linear indices of PCC symbols for this packet configuration
            const std::vector<uint32_t> k_i_linear_single = get_k_i_linear(b, N_TS);

            dectnrp_assert(k_i_linear_single.size() == constants::pcc_cells,
                           "Incorrect number of PCC linear indices k_i");

            // convert linear indices to symbol and subcarrier indices
            const std::vector<uint32_t> l_per_symbol_single =
                idx2sub_col(k_i_linear_single, N_b_DFT);
            const common::vec2d<uint32_t> k_i_per_symbol_single =
                idx2sub_row(k_i_linear_single, N_b_DFT, N_guards);

            dectnrp_assert(l_per_symbol_single.size() == k_i_per_symbol_single.size(),
                           "Incorrect number of PCC OFDM sybmols.");

            // assert
            uint32_t sum = 0;
            for (uint32_t i = 0; i < l_per_symbol_single.size(); ++i) {
                sum += k_i_per_symbol_single[i].size();
            }
            dectnrp_assert(sum == constants::pcc_cells, "Incorrect number of PCC OFDM symbols");

            // attach
            l_all_symbols[b_idx].push_back(l_per_symbol_single);
            k_i_all_symbols[b_idx].push_back(k_i_per_symbol_single);
        }
    }
}

void pcc_t::set_configuration(const uint32_t b, const uint32_t N_TS) {
    config_local_cnt = 0;
    ptr_l_all_symbols = &l_all_symbols[phyres::b2b_idx[b]][phyres::N_TS_2_N_TS_idx_vec[N_TS]];
    ptr_k_i_all_symbols = &k_i_all_symbols[phyres::b2b_idx[b]][phyres::N_TS_2_N_TS_idx_vec[N_TS]];
}

bool pcc_t::is_symbol_index(const uint32_t l) {
    // all PCC symbols processed?
    if (config_local_cnt >= ptr_l_all_symbols->size()) {
        return false;
    }

    // check if this is a PCC symbol index
    if (ptr_l_all_symbols->at(config_local_cnt) == l) {
        ++config_local_cnt;
        return true;
    }

    return false;
}

const std::vector<uint32_t>& pcc_t::get_k_i_one_symbol() const {
    // it must be minus one because config_local_cnt is incremented in function
    // is_symbol_index()
    return ptr_k_i_all_symbols->at(config_local_cnt - 1);
}

std::vector<uint32_t> pcc_t::get_k_i_linear(const uint32_t b, const uint32_t N_TS) {
    // get packet dimensions
    const uint32_t b_idx = phyres::b2b_idx[b];
    const uint32_t N_b_DFT = phyres::N_b_DFT_lut[b_idx];
    const uint32_t guards_top = phyres::guards_top_lut[b_idx];
    const uint32_t guards_bottom = phyres::guards_bottom_lut[b_idx];

    // PCC will definitely be contained within this amount of OFDM symbols for any packet
    // configuration.
    const uint32_t N_PACKET_symb_MINIMUM = 20;

    /* The actual value of u is irrelevant, but it has to be defined for DRS. This is because u
     * determines the number of zero symbols at the end of the packet, which can collide with DRS.
     */
    const uint32_t u_irrelevant = 8;

    // we will closely follow the algorithm in 5.2.4, for this we create a virtual frame
    std::vector<int32_t> virtual_frame(N_PACKET_symb_MINIMUM * N_b_DFT);

    // fill each subcarrier with its linear index
    for (uint32_t l = 0; l < N_PACKET_symb_MINIMUM; ++l) {
        for (uint32_t i = 0; i < N_b_DFT; ++i) {
            virtual_frame.at(l * N_b_DFT + i) = l * N_b_DFT + i;
        }
    }

    // mark DC subcarrier in virtual frame
    for (uint32_t l = 0; l < N_PACKET_symb_MINIMUM; ++l) {
        virtual_frame.at(l * N_b_DFT + N_b_DFT / 2) = PCC_OCCUPIED_DC;
    }

    // mark guard subcarriers in virtual frame
    for (uint32_t l = 0; l < N_PACKET_symb_MINIMUM; ++l) {
        for (uint32_t i = 0; i < guards_bottom; ++i) {
            virtual_frame.at(l * N_b_DFT + i) = PCC_OCCUPIED_GUARDS;
        }
        for (uint32_t i = N_b_DFT - guards_top; i < N_b_DFT; ++i) {
            virtual_frame.at(l * N_b_DFT + i) = PCC_OCCUPIED_GUARDS;
        }
    }

    // to exclude DRS subcarriers we need DRS symbol positions
    const common::vec2d<uint32_t> k_i_linear =
        drs_t::get_k_i_linear(u_irrelevant, b, N_PACKET_symb_MINIMUM, N_TS);

    // mark DRS subcarriers in virtual_frame
    int32_t* virtual_frame_ptr = virtual_frame.data();
    // for each transmit stream
    for (uint32_t t = 0; t < k_i_linear.size(); ++t) {
        // for each subcarrier
        for (uint32_t i_linear : k_i_linear[t]) {
            virtual_frame_ptr[i_linear] = PCC_OCCUPIED_DRS;
        }
    }

    // here we start the actual algorithm according to 5.2.4

    // final set of PCC subcarriers
    std::vector<uint32_t> k_PCC_l;

    // step 1)
    uint32_t l = 1;
    uint32_t N_unalloc_subc = constants::pcc_cells;

    while (1 == 1) {
        // step 2)
        std::vector<uint32_t> k_0_U_l;
        for (uint32_t i = 0; i < N_b_DFT; ++i) {
            // not DC or guard or DRS?
            if (virtual_frame.at(l * N_b_DFT + i) >= 0) {
                // add linear index
                k_0_U_l.push_back(virtual_frame.at(l * N_b_DFT + i));
            }
        }

        // step 3)
        const uint32_t U = k_0_U_l.size();
        if (U < N_unalloc_subc) {
            // step 4 a)
            for (uint32_t u = 0; u < U; ++u) {
                k_PCC_l.push_back(k_0_U_l[u]);
            }

            // step 4 b)
            l = l + 1;
            N_unalloc_subc = N_unalloc_subc - U;

            // step 4 c)
            continue;
        } else {
            // step 5)
            const uint32_t R_PCC = 7;

            // step 6)
            const uint32_t C_PCC = U / R_PCC;

            dectnrp_assert(U % R_PCC == 0, "Incorrect number of subcarriers U");

            // step 7) create and fill matrix
            std::vector<uint32_t> mat(R_PCC * C_PCC);
            uint32_t* mat_ptr = mat.data();
            for (uint32_t u = 0; u < U; ++u) {
                mat_ptr[u] = k_0_U_l[u];
            }

            // step 8)
            for (uint32_t c = 0; c < C_PCC; ++c) {
                for (uint32_t r = 0; r < R_PCC; ++r) {
                    k_PCC_l.push_back(mat.at(r * C_PCC + c));

                    N_unalloc_subc--;

                    // do we have 98 subcarriers?
                    if (N_unalloc_subc == 0) {
                        break;
                    }
                }

                // abort prematurely
                if (N_unalloc_subc == 0) {
                    break;
                }
            }

            dectnrp_assert(k_PCC_l.size() == constants::pcc_cells,
                           "Incorrect number of PCC subcarriers");

            // NOTE
            std::sort(k_PCC_l.begin(), k_PCC_l.end());

            // leave while loop
            break;
        }
    }

    return k_PCC_l;
}

std::vector<uint32_t> pcc_t::idx2sub_col(const std::vector<uint32_t> linear_idx,
                                         uint32_t dim_column) {
    std::vector<uint32_t> column_idx_vec;

    if (linear_idx.empty()) {
        return column_idx_vec;
    }

    // add index of first column
    column_idx_vec.push_back(linear_idx[0] / dim_column);

    // go over each symbols and check, if we have a new column index
    for (uint32_t i = 0; i < linear_idx.size(); ++i) {
        // in which column is this linear index located?
        const uint32_t column_idx = linear_idx[i] / dim_column;

        // is this a new column index? if so, attach
        if (column_idx != column_idx_vec.back()) {
            column_idx_vec.push_back(column_idx);
        }
    }

    return column_idx_vec;
}

common::vec2d<uint32_t> pcc_t::idx2sub_row(const std::vector<uint32_t> linear_idx,
                                           uint32_t dim_column,
                                           uint32_t cnt_offset) {
    // create container
    std::vector<uint32_t> column_idx_vec;
    common::vec2d<uint32_t> linear_idx_per_column;

    if (linear_idx.empty()) {
        return linear_idx_per_column;
    }

    // add index of first column
    column_idx_vec.push_back(linear_idx[0] / dim_column);
    linear_idx_per_column.push_back(std::vector<uint32_t>());

    // fill the rows
    for (uint32_t i = 0; i < linear_idx.size(); ++i) {
        // in which column is this linear index located?
        const uint32_t column_idx = linear_idx[i] / dim_column;

        // is this a new column index? if so, attach
        if (column_idx != column_idx_vec.back()) {
            column_idx_vec.push_back(column_idx);
            linear_idx_per_column.push_back(std::vector<uint32_t>());
        }

        uint32_t k_i = linear_idx[i] - column_idx * dim_column;

        // start counting zero at particular offset
        k_i = k_i - cnt_offset;

        // add new subcarrier index
        linear_idx_per_column[linear_idx_per_column.size() - 1].push_back(k_i);
    }

    // assert
    for (uint32_t i = 0; i < linear_idx_per_column.size(); ++i) {
        dectnrp_assert(!linear_idx_per_column[i].empty(),
                       "No indices for one of the columns written.");
    }

    return linear_idx_per_column;
}

}  // namespace dectnrp::sp3
