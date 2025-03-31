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

#include "dectnrp/sections_part3/drs.hpp"

#include <algorithm>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part3/pdc.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::section3 {

drs_t::drs_t(const uint32_t b_max, const uint32_t N_TS_max) {
    k_i_for_1_2.clear();
    k_i_for_6_or_11_12.clear();

    y_DRS_i_TS_0_to_3.clear();
    y_DRS_i_TS_4_to_7.clear();

    const auto k_b_OCC_vec = phyres::get_k_b_OCC_vec();

    uint32_t b_idx_max = phyres::b2b_idx[b_max];

    for (uint32_t b_idx = 0; b_idx <= b_idx_max; ++b_idx) {
        // add new row
        k_i_for_1_2.push_back(common::vec2d<uint32_t>());
        k_i_for_6_or_11_12.push_back(common::vec2d<uint32_t>());

        // go over each transmit stream
        for (uint32_t t = 0; t < std::min(uint32_t(4), N_TS_max); ++t) {
            uint32_t n = 0;

            k_i_for_1_2[b_idx].push_back(get_k_i_single(k_b_OCC_vec, b_idx, t, n));

            n = 1;

            k_i_for_6_or_11_12[b_idx].push_back(get_k_i_single(k_b_OCC_vec, b_idx, t, n));
        }

        // add new row
        y_DRS_i_TS_0_to_3.push_back(common::vec2d<cf_t>());
        y_DRS_i_TS_4_to_7.push_back(common::vec2d<cf_t>());

        // go over each transmit stream
        for (uint32_t t = 0; t < std::min(N_TS_max, uint32_t(4)); ++t) {
            y_DRS_i_TS_0_to_3[b_idx].push_back(get_Y_DRS_i_single(b_idx, t));
        }

        for (uint32_t t = 4; t < N_TS_max; ++t) {
            y_DRS_i_TS_4_to_7[b_idx].push_back(get_Y_DRS_i_single(b_idx, t));
        }
    }
}

void drs_t::set_configuration(const uint32_t b, const uint32_t N_TS) {
    config_b_idx = phyres::b2b_idx[b];
    config_N_TS = N_TS;

    // DRS always starts in the first OFDM symbol index
    l_next = 1;

    k_i_effective = &k_i_for_1_2[config_b_idx];
    y_DRS_i_effective = &y_DRS_i_TS_0_to_3[config_b_idx];

    // we always start by copying into the lower transmit streams
    TS_idx_first_effective = 0;
}

bool drs_t::is_symbol_index(const uint32_t l) { return l == l_next; }

const common::vec2d<uint32_t>& drs_t::get_k_i() { return *k_i_effective; }

const common::vec2d<cf_t>& drs_t::get_y_DRS_i() { return *y_DRS_i_effective; }

uint32_t drs_t::get_ts_idx_first() { return TS_idx_first_effective; }

uint32_t drs_t::get_ts_idx_last_and_advance() {
    // last TS index for current OFDM symbol
    const uint32_t TS_idx_last_effective =
        (TS_idx_first_effective == 0) ? std::min(config_N_TS - 1, uint32_t(3)) : 7;

    // what is the next symbol index we will be looking for? see Figure 4.5-2 and Figure 4.5-3
    if (config_N_TS <= 2) {
        l_next += 5;

        switch_k_i_pointer();
    } else if (config_N_TS == 4) {
        l_next += 10;

        switch_k_i_pointer();
    }
    // config_N_TS == 8
    else {
        if (l_next % 2 == 1) {
            l_next += 1;
        } else {
            l_next += 9;
            switch_k_i_pointer();
        }

        switch_y_DRS_i_pointer();

        TS_idx_first_effective = (TS_idx_first_effective == 0) ? 4 : 0;
    }

    return TS_idx_last_effective;
}

common::vec2d<uint32_t> drs_t::get_k_i_linear(const uint32_t u,
                                              const uint32_t b,
                                              const uint32_t N_PACKET_symb,
                                              const uint32_t N_TS) {
    dectnrp_assert(
        !(N_TS == 4 && N_PACKET_symb < 15),
        "N_TS = 4, the value of N_PACKET_symb must be at least 15 to accomodate two DRS symbols");

    dectnrp_assert(
        !(u == 8 && N_TS == 8 && (N_PACKET_symb < 20 || N_PACKET_symb % 10 != 0)),
        "u==8 and N_TS = 8, the value of N_PACKET_symb must be at least 20 and a multiple of 10");

    const auto k_b_OCC_vec = phyres::get_k_b_OCC_vec();

    common::vec2d<uint32_t> k_i_linear;

    const uint32_t b_idx = phyres::b2b_idx[b];
    const uint32_t N_b_DFT = phyres::N_b_DFT_lut[b_idx];
    const uint32_t N_b_OCC = phyres::N_b_OCC_lut[b_idx];

    const uint32_t N_step = get_N_step(N_TS);

    const uint32_t n_subc = N_b_OCC / 4;

    // N_TS = N_eff_TX
    const uint32_t n_symb = pdc_t::get_nof_OFDM_symbols_carrying_DRS_per_TS(u, N_PACKET_symb, N_TS);

    // go over each transmit stream
    uint32_t N_DRS_subc_cnt = 0;
    for (uint32_t t = 0; t < N_TS; ++t) {
        // add new row
        k_i_linear.push_back(std::vector<uint32_t>(n_subc * n_symb));

        uint32_t idx = 0;
        for (uint32_t n = 0; n <= n_symb - 1; ++n) {
            uint32_t l = 1 + t / 4 + n * N_step;

            // N_b_DFT/2 because we want indices to start at zero instead of -N_b_DFT/2
            const int linear_offset = N_b_DFT / 2 + N_b_DFT * l;

            for (uint32_t i = 0; i <= n_subc - 1; ++i) {
                k_i_linear[t][idx] =
                    k_b_OCC_vec.b_x[b_idx][i * 4 + (t + (n % 2) * 2) % 4] + linear_offset;
                ++idx;
                ++N_DRS_subc_cnt;
            }
        }
    }

    // assert
    const uint32_t N_eff_TX = N_TS;  // Table 7.2-1
    const uint32_t N_DRS_subc = pdc_t::get_N_DRS_subc(u, N_PACKET_symb, N_eff_TX, N_b_OCC);
    dectnrp_assert(N_DRS_subc_cnt == N_DRS_subc, "Incorrect number of DRS symbols");

    return k_i_linear;
}

uint32_t drs_t::get_N_step(const uint32_t N_TS_or_N_eff_TX) {
    return (N_TS_or_N_eff_TX <= 2) ? 5 : 10;
}

uint32_t drs_t::get_nof_drs_subc(const uint32_t b) { return b * 14; }

void drs_t::switch_k_i_pointer() {
    if (k_i_effective == &k_i_for_1_2[config_b_idx]) {
        k_i_effective = &k_i_for_6_or_11_12[config_b_idx];
    } else {
        k_i_effective = &k_i_for_1_2[config_b_idx];
    }
}

void drs_t::switch_y_DRS_i_pointer() {
    if (y_DRS_i_effective == &y_DRS_i_TS_0_to_3[config_b_idx]) {
        y_DRS_i_effective = &y_DRS_i_TS_4_to_7[config_b_idx];
    } else {
        y_DRS_i_effective = &y_DRS_i_TS_0_to_3[config_b_idx];
    }
}

std::vector<uint32_t> drs_t::get_k_i_single(const phyres::k_b_OCC_vec_t& k_b_OCC_vec,
                                            const uint32_t b_idx,
                                            const uint32_t t,
                                            const uint32_t n) {
    std::vector<uint32_t> k_i;

    const int32_t N_b_OCC = phyres::N_b_OCC_lut[b_idx];

    // fill k_i
    for (int32_t i = 0; i <= N_b_OCC / 4 - 1; ++i) {
        int32_t index = k_b_OCC_vec.b_x[b_idx][i * 4 + (t + (n % 2) * 2) % 4];

        // we want the lowest possible index to be zero
        index += N_b_OCC / 2;

        k_i.push_back(static_cast<uint32_t>(index));
    }

    return k_i;
}

std::vector<cf_t> drs_t::get_Y_DRS_i_single(const uint32_t b_idx, const uint32_t t) {
    std::vector<cf_t> y_DRS_i;

    const uint32_t N_b_OCC = phyres::N_b_OCC_lut[b_idx];

    // fill y_DRS_i

    // error in standard, should be  t<4, so t=0,1,2 and 3
    // if(t<=4){
    if (t < 4) {
        for (uint32_t i = 0; i <= N_b_OCC / 4 - 1; ++i) {
            const cf_t value = y_b_1[(4 * i + t % 4) % 56];

            y_DRS_i.push_back(value);
        }
    }
    // error in standart, should be  t>=4, so t=4, 5, 6 and 7
    // else if(t>4){
    else if (t >= 4) {
        for (uint32_t i = 0; i <= N_b_OCC / 4 - 1; ++i) {
            const cf_t value = -y_b_1[(4 * i + t % 4) % 56];

            y_DRS_i.push_back(value);
        }
    }

    return y_DRS_i;
}

}  // namespace dectnrp::section3
