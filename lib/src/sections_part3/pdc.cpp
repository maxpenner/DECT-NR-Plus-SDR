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

#include "dectnrp/sections_part3/pdc.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/sections_part3/drs.hpp"
#include "dectnrp/sections_part3/pcc.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

// marks subcarriers as occupied, must be negative
static constexpr int32_t PDC_OCCUPIED_DC = -1;
static constexpr int32_t PDC_OCCUPIED_GUARDS = -2;
static constexpr int32_t PDC_OCCUPIED_DRS = -3;
static constexpr int32_t PDC_OCCUPIED_PCC = -4;

static constexpr uint32_t PDC_N_TS_1_2_4__8_OFDM_SYMBOL_INDEX_FULLY_OCCUPIED = 10;
static constexpr uint32_t PDC_N_TS_1_2_4__8_OFDM_SYMBOL_INDEX_REPETITION_END = 21;

namespace dectnrp::sp3 {

pdc_t::pdc_t(const uint32_t b_max, const uint32_t N_TS_max) {
    l_all_symbols.clear();
    k_i_all_symbols.clear();

    const uint32_t b_idx_max = phyres::b2b_idx[b_max];

    // for each value of beta
    for (uint32_t b_idx = 0; b_idx <= b_idx_max; ++b_idx) {
        const uint32_t b = phyres::b_idx2b[b_idx];
        const uint32_t N_b_DFT = phyres::N_b_DFT_lut[b_idx];
        const uint32_t N_b_OCC = phyres::N_b_OCC_lut[b_idx];
        const uint32_t N_guards = phyres::guards_bottom_lut[b_idx];

        // add new rows
        l_all_symbols.push_back(common::vec2d<uint32_t>());
        k_i_all_symbols.push_back(common::vec3d<uint32_t>());

        // we have 1, 2, 4 or 8 transmit streams, each containing PDC subcarrier
        for (uint32_t N_TS = 1; N_TS <= N_TS_max; N_TS = N_TS * 2) {
            // determine linear indices of PDC symbols for this packet configuration, the assumed
            // length of the packet is N_PACKET_symb = 20
            const std::vector<uint32_t> k_i_linear_single = get_k_i_linear(b, N_TS);

            // convert linear indices to symbol and subcarrier indices per symbol, include empty
            // symbols
            std::vector<uint32_t> l_per_symbol_single =
                idx2sub_col_zero_vec(k_i_linear_single, N_b_DFT);
            common::vec2d<uint32_t> k_i_per_symbol_single =
                idx2sub_row_zero_vec(k_i_linear_single, N_b_DFT, N_guards);

            dectnrp_assert(
                PDC_N_TS_1_2_4__8_OFDM_SYMBOL_INDEX_REPETITION_END <= l_per_symbol_single.size(),
                "More OFDM symbols are required to fully cover the repetition range.");

            dectnrp_assert(l_per_symbol_single.size() == k_i_per_symbol_single.size(),
                           "Incorrect number of PDC OFDM sybmols.");

            // erase all irrelevant OFDM symbol indices
            l_per_symbol_single.erase(
                l_per_symbol_single.begin() + PDC_N_TS_1_2_4__8_OFDM_SYMBOL_INDEX_REPETITION_END,
                l_per_symbol_single.end());
            k_i_per_symbol_single.erase(
                k_i_per_symbol_single.begin() + PDC_N_TS_1_2_4__8_OFDM_SYMBOL_INDEX_REPETITION_END,
                k_i_per_symbol_single.end());

            // we don't actually need to save the subcarrier indices for fully occupied vector, we
            // need to keep only one of them
            for (uint32_t i = 0; i < k_i_per_symbol_single.size(); ++i) {
                // we keep one fully occupied vector
                if (i == PDC_N_TS_1_2_4__8_OFDM_SYMBOL_INDEX_FULLY_OCCUPIED) {
                    continue;
                }

                // all the other fully occupied symbols are erased, we just leave one subcarrier to
                // identify these subcarriers
                if (k_i_per_symbol_single[i].size() == N_b_OCC) {
                    k_i_per_symbol_single[i].erase(k_i_per_symbol_single[i].begin() + 1,
                                                   k_i_per_symbol_single[i].end());
                }
            }

            // attach
            l_all_symbols[b_idx].push_back(l_per_symbol_single);
            k_i_all_symbols[b_idx].push_back(k_i_per_symbol_single);
        }
    }
}

void pdc_t::set_configuration(const uint32_t b, const uint32_t N_TS) {
    config_b_idx = phyres::b2b_idx[b];
    config_N_TS = N_TS;
    config_N_TS_idx = phyres::N_TS_2_N_TS_idx_vec[N_TS];

    k_i_all_symbols_effective = &k_i_all_symbols[config_b_idx][config_N_TS_idx];

    if (config_N_TS <= 2) {
        l_limit = 6;    // repetition range starts at this index
        l_repeat = 10;  // the range from 6 to 15 includes 10 symbols
    } else if (config_N_TS == 4) {
        l_limit = 11;
        l_repeat = 10;
    } else if (config_N_TS == 8) {
        l_limit = 11;
        l_repeat = 10;
    }
}

bool pdc_t::is_symbol_index(const uint32_t l) {
    // this is true for all OFDM symbols with index 0 to l_limit
    if (l <= l_limit) {
        l_equivalent = l;
    }
    // For indices at or above l_limit we have to find an equivalent in the repetition range.
    // Examples:
    // For example, with N_TS = 1 the symbol index 7 is equivalent to symbol index 2.
    // For example, with N_TS = 1 the symbol index 11 is equivalent to symbol index 11.
    // For example, with N_TS = 1 the symbol index 16 is equivalent to symbol index 11.
    // For example, with N_TS = 1 the symbol index 12 is equivalent to symbol index 12.
    // For example, with N_TS = 1 the symbol index 17 is equivalent to symbol index 12.
    else {
        l_equivalent = l - ((l - l_limit) / l_repeat) * l_repeat;
    }

    // is the symbol index marked as fully occupied by leaving only one subcarrier index?
    if (k_i_all_symbols_effective->at(l_equivalent).size() == 1) {
        l_equivalent = PDC_N_TS_1_2_4__8_OFDM_SYMBOL_INDEX_FULLY_OCCUPIED;
    }

    return !k_i_all_symbols_effective->at(l_equivalent).empty();
}

const std::vector<uint32_t>& pdc_t::get_k_i_one_symbol() const {
    return k_i_all_symbols_effective->at(l_equivalent);
}

uint32_t pdc_t::get_N_DF_symb(const uint32_t u, const uint32_t N_PACKET_symb) {
    if (u == 1) {
        return N_PACKET_symb - 2;
    } else if (u == 2 || u == 4) {
        return N_PACKET_symb - 3;
    }
    // u==8
    else {
        return N_PACKET_symb - 4;
    }
}

uint32_t pdc_t::get_nof_OFDM_symbols_carrying_DRS_per_TS(const uint32_t u,
                                                         const uint32_t N_PACKET_symb,
                                                         const uint32_t N_eff_TX) {
    dectnrp_assert(
        !(N_eff_TX == 4 && N_PACKET_symb < 15),
        "N_eff_TX = 4, the value of N_PACKET_symb must be at least 15 to accomodate two DRS symbols");

    dectnrp_assert(
        !(u == 8 && N_eff_TX == 8 && (N_PACKET_symb < 20 || N_PACKET_symb % 10 != 0)),
        "u==8 and N_eff_TX = 8, the value of N_PACKET_symb must be at least 20 and a multiple of 10");

    const uint32_t N_step = (N_eff_TX <= 2) ? 5 : 10;

    uint32_t nof_OFDM_symbol_carying_DRS = N_PACKET_symb / N_step;

    if (N_step == 10 && N_PACKET_symb % 10 != 0) {
        dectnrp_assert(N_PACKET_symb % 5 == 0, "N_PACKET_symb not a multiple of 5 or 10.");

        /* When N_step=10, the equation floor(N_PACKET_symb/N_step)=floor(15/10)=1 gives us the
         * incorrect number of DRS symbols. This can be seen in Figure 4.5-3 d). For odd multiples
         * of 5 we have to add one more symbol.
         */
        ++nof_OFDM_symbol_carying_DRS;
    }

    return nof_OFDM_symbol_carying_DRS;
}

uint32_t pdc_t::get_N_DRS_subc(const uint32_t u,
                               const uint32_t N_PACKET_symb,
                               const uint32_t N_eff_TX,
                               const uint32_t N_b_OCC) {
    return N_eff_TX * N_b_OCC / 4 *
           get_nof_OFDM_symbols_carrying_DRS_per_TS(u, N_PACKET_symb, N_eff_TX);
}

uint32_t pdc_t::get_N_PDC_subc(const uint32_t N_PACKET_symb,
                               const uint32_t u,
                               const uint32_t N_eff_TX,
                               const uint32_t N_b_OCC) {
    const uint32_t N_PCC_subc = constants::pcc_cells;

    // N_PACKET_symb should always be >= 5, so the minimum value for N_DF_symb is 1
    const uint32_t N_DF_symb = get_N_DF_symb(u, N_PACKET_symb);

    const uint32_t N_DRS_subc = get_N_DRS_subc(u, N_PACKET_symb, N_eff_TX, N_b_OCC);

    if (N_DF_symb * N_b_OCC <= (N_DRS_subc + N_PCC_subc)) {
        return 0;
    }

    return N_DF_symb * N_b_OCC - N_DRS_subc - N_PCC_subc;
}

std::vector<uint32_t> pdc_t::get_k_i_linear(const uint32_t b, const uint32_t N_TS) {
    std::vector<uint32_t> k_i_linear;

    // get packet dimensions
    const uint32_t b_idx = phyres::b2b_idx[b];
    const uint32_t N_b_DFT = phyres::N_b_DFT_lut[b_idx];
    const uint32_t N_b_OCC = phyres::N_b_OCC_lut[b_idx];
    const uint32_t guards_top = phyres::guards_top_lut[b_idx];
    const uint32_t guards_bottom = phyres::guards_bottom_lut[b_idx];

    // We assume a sufficently large packet.
    // We need 22 to cover the repetition range when N_eff_TX=8. However, with N_eff_TX the value of
    // N_PACKET_symbol must be a multiple of 10, therefore it is 30. Therefore, do not change the
    // value of N_PACKET_symb=30.
    const uint32_t N_PACKET_symb = 30;
    const uint32_t u = 8;

    // create a virtual frame
    std::vector<int32_t> virtual_frame(N_PACKET_symb * N_b_DFT);

    // fill each subcarrier with its linear index
    for (uint32_t l = 0; l < N_PACKET_symb; ++l) {
        for (uint32_t i = 0; i < N_b_DFT; ++i) {
            virtual_frame.at(l * N_b_DFT + i) = l * N_b_DFT + i;
        }
    }

    // mark DC subcarriers in virtual frame
    for (uint32_t l = 0; l < N_PACKET_symb; ++l) {
        virtual_frame.at(l * N_b_DFT + N_b_DFT / 2) = PDC_OCCUPIED_DC;
    }

    // mark guard subcarriers in virtual frame
    for (uint32_t l = 0; l < N_PACKET_symb; ++l) {
        for (uint32_t i = 0; i < guards_bottom; ++i) {
            virtual_frame.at(l * N_b_DFT + i) = PDC_OCCUPIED_GUARDS;
        }
        for (uint32_t i = N_b_DFT - guards_top; i < N_b_DFT; ++i) {
            virtual_frame.at(l * N_b_DFT + i) = PDC_OCCUPIED_GUARDS;
        }
    }

    // load linear indices of DRS
    const uint32_t N_eff_TX = N_TS;  // Table 7.2-1
    const common::vec2d<uint32_t> k_i_linear_drs =
        drs_t::get_k_i_linear(u, b, N_PACKET_symb, N_eff_TX);

    // mark DRS subcarriers in virtual_frame
    int32_t* virtual_frame_ptr = virtual_frame.data();
    // for each transmit stream
    for (uint32_t t = 0; t < k_i_linear_drs.size(); ++t) {
        // for each subcarrier
        for (uint32_t i_linear : k_i_linear_drs[t]) {
            virtual_frame_ptr[i_linear] = PDC_OCCUPIED_DRS;
        }
    }

    // mark PCC subcarriers in virtual_frame
    const std::vector<uint32_t> k_i_linear_pcc = pcc_t::get_k_i_linear(b, N_TS);
    for (uint32_t i_linear : k_i_linear_pcc) {
        virtual_frame_ptr[i_linear] = PDC_OCCUPIED_PCC;
    }

    // here we start the actual algorithm according to 5.2.5

    const uint32_t N_DF_symb = get_N_DF_symb(u, N_PACKET_symb);

    // how many subcarriers will there be for PDC according to 5.2.5?
    const uint32_t N_PDC_subc = get_N_PDC_subc(N_PACKET_symb, u, N_eff_TX, N_b_OCC);

    dectnrp_assert(N_PDC_subc > 0, "N_PDC_subc is zero");

    // allocate memory
    k_i_linear.resize(N_PDC_subc);

    // go through entire frame and check if subcarrier is available for PDC
    uint32_t N_PDC_subc_check = 0;
    for (uint32_t l = 1; l < 1 + N_DF_symb; ++l) {
        for (uint32_t i = 0; i < N_b_DFT; ++i) {
            // occupied subcarrier are all negative
            if (virtual_frame.at(l * N_b_DFT + i) >= 0) {
                k_i_linear.at(N_PDC_subc_check++) = {(uint32_t)virtual_frame.at(l * N_b_DFT + i)};
            }
        }
    }

    dectnrp_assert(N_PDC_subc == N_PDC_subc_check, "N_PDC counter has incorrect final value");

    return k_i_linear;
}

std::vector<uint32_t> pdc_t::idx2sub_col_zero_vec(const std::vector<uint32_t> linear_idx,
                                                  const uint32_t dim_column) {
    // create container
    std::vector<uint32_t> column_idx_vec;

    const int32_t column_last = linear_idx.back() / dim_column;

    // create correct number of rows
    for (int32_t i = 0; i <= column_last; ++i) {
        column_idx_vec.push_back((uint32_t)i);
    }

    return column_idx_vec;
}

common::vec2d<uint32_t> pdc_t::idx2sub_row_zero_vec(const std::vector<uint32_t> linear_idx,
                                                    const uint32_t dim_column,
                                                    const uint32_t offset) {
    // create container
    common::vec2d<uint32_t> linear_idx_per_column;

    const uint32_t column_last = linear_idx.back() / dim_column;

    // create correct number of rows
    for (uint32_t i = 0; i <= column_last; ++i) {
        linear_idx_per_column.push_back(std::vector<uint32_t>());
    }

    // fill the rows
    for (uint32_t i = 0; i < linear_idx.size(); ++i) {
        const int32_t column_idx = linear_idx[i] / dim_column;
        int32_t k_i = linear_idx[i] - column_idx * dim_column;

        // start counting zero at particular offset
        k_i = k_i - offset;

        // add new subcarrier index
        linear_idx_per_column[column_idx].push_back(k_i);
    }

    return linear_idx_per_column;
}

}  // namespace dectnrp::sp3
