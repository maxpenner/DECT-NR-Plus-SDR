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

#include "dectnrp/sections_part3/stf.hpp"

#include <volk/volk.h>

#include <algorithm>
#include <bit>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::sp3 {

stf_t::stf_t(const uint32_t b_max, const uint32_t N_eff_TX_max, const float scale_) {
    scale = scale_;

    // load physical dimensions
    const auto k_b_OCC = phyres::get_k_b_OCC();

    // create LUT
    for (uint32_t b_idx = 0; b_idx <= phyres::b2b_idx[b_max]; ++b_idx) {
        // total number of occupied subcarriers
        const uint32_t N_b_OCC = phyres::N_b_OCC_lut[b_idx];

        // subcarriers indices of STF as subset of the occupied subcarriers
        std::vector<int32_t> k_i(N_b_OCC / 4);
        switch (b_idx) {
            case 0:
                fill_k_i(k_i.data(), k_b_OCC.b_1, N_b_OCC);
                break;
            case 1:
                fill_k_i(k_i.data(), k_b_OCC.b_2, N_b_OCC);
                break;
            case 2:
                fill_k_i(k_i.data(), k_b_OCC.b_4, N_b_OCC);
                break;
            case 3:
                fill_k_i(k_i.data(), k_b_OCC.b_8, N_b_OCC);
                break;
            case 4:
                fill_k_i(k_i.data(), k_b_OCC.b_12, N_b_OCC);
                break;
            case 5:
                fill_k_i(k_i.data(), k_b_OCC.b_16, N_b_OCC);
                break;
        }

        // complex values of STF
        std::vector<cf_t> y_STF_i(N_b_OCC / 4);

        // add new row
        y_STF_filled_b_x.push_back(common::vec2d<cf_t>());

        // y_STF_i
        for (uint32_t N_eff_TX = 1; N_eff_TX <= N_eff_TX_max; N_eff_TX = N_eff_TX * 2) {
            // different for every N_eff_TX
            fill_y_STF_i(y_STF_i.data(), N_b_OCC, N_eff_TX, scale);

            // fill lookup vector
            y_STF_filled_b_x[b_idx].push_back(
                fill_y_STF_filled(y_STF_i.data(), k_i.data(), N_b_OCC));
        }
    }
}

const std::vector<cf_t>& stf_t::get_stf(const uint32_t b, const uint32_t N_eff_TX) const {
    return y_STF_filled_b_x[phyres::b2b_idx[b]][phyres::N_TS_2_N_TS_idx_vec[N_eff_TX]];
}

std::vector<float> stf_t::get_cover_sequence(const uint32_t u) {
    dectnrp_assert(std::has_single_bit(u) && u <= 8, "u undefined");

    const uint32_t N_stf_pattern = get_N_stf_pattern(u);

    std::vector<float> ret(N_stf_pattern);

    for (uint32_t i = 0; i < N_stf_pattern; ++i) {
        ret.at(i) = cover_sequence.at(i);
    }

    return ret;
}

void stf_t::apply_cover_sequence(cf_t* dst,
                                 const cf_t* src,
                                 const uint32_t u,
                                 const uint32_t N_samples_stf_pattern_os) {
    const uint32_t nof_pattern = get_N_stf_pattern(u);

    // volk_32f_s32f_multiply_32f uses floats, we input complex floats
    const uint32_t nof_floats = N_samples_stf_pattern_os * 2;

    // apply cover sequence for this pattern ...
    for (uint32_t pattern_idx = 0; pattern_idx < nof_pattern; ++pattern_idx) {
        // ... unless it's one, then skip multiplication
        if (cover_sequence[pattern_idx] == 1.0f) {
            continue;
        }

        const uint32_t offset = pattern_idx * N_samples_stf_pattern_os;

        volk_32f_s32f_multiply_32f((float*)&dst[offset],
                                   (const float*)&src[offset],
                                   cover_sequence[pattern_idx],
                                   nof_floats);
    }
}

void stf_t::apply_cover_sequence(std::vector<cf_t*>& dst,
                                 const std::vector<cf_t*>& src,
                                 const uint32_t N_TX,
                                 const uint32_t u,
                                 const uint32_t N_samples_stf_pattern_os) {
    // go over antennas to read memory sequentially
    for (uint32_t ant_idx = 0; ant_idx < N_TX; ++ant_idx) {
        apply_cover_sequence(dst[ant_idx], src[ant_idx], u, N_samples_stf_pattern_os);
    }
}

std::vector<float> stf_t::get_cover_sequence_pairwise_product(const uint32_t u) {
    const std::vector<float> cs = get_cover_sequence(u);

    dectnrp_assert(
        cs.size() == constants::N_stf_pattern_u1 || cs.size() == constants::N_stf_pattern_u248,
        "incorrect number of patterns");

    std::vector<float> cscs(cs.size() - 1);

    for (uint32_t i = 0; i < cscs.size(); ++i) {
        cscs.at(i) = get_cover_sequence_pairwise_product_single(i);
    }

    dectnrp_assert(
        !std::any_of(
            cscs.begin(), cscs.end(), [](const float val) { return std::abs(val) != 1.0f; }),
        "incorrect cover sequence pairwise product");

    return cscs;
}

const std::vector<float> stf_t::y_b_1 = {1, -1, 1, 1, -1, 1, 1, -1, 1, 1, 1, -1, -1, -1};

const std::vector<float> stf_t::y_b_2 = {-1, 1,  -1, 1, 1,  -1, 1,  1, -1, 1, 1,  1,  -1, 1,
                                         -1, -1, -1, 1, -1, -1, -1, 1, 1,  1, -1, -1, -1, -1};

const std::vector<float> stf_t::y_b_4 = {-1, -1, -1, 1,  -1, 1,  -1, -1, 1,  1,  1,  1,  -1, 1,
                                         -1, -1, -1, 1,  -1, 1,  1,  -1, -1, -1, -1, -1, 1,  -1,
                                         1,  1,  1,  -1, 1,  -1, 1,  1,  -1, -1, -1, -1, 1,  -1,
                                         -1, -1, -1, 1,  -1, 1,  1,  -1, -1, -1, -1, -1, 1,  -1};

void stf_t::fill_k_i(int* k_i, const int* k_b_OCC, const int N_b_OCC) {
    int idx = 0;

    for (int i = 0; i <= N_b_OCC / 8 - 1; ++i) {
        k_i[idx++] = k_b_OCC[i * 4];
    }

    for (int i = N_b_OCC / 8; i <= N_b_OCC / 4 - 1; ++i) {
        k_i[idx++] = k_b_OCC[N_b_OCC / 2 + 3 + (i - N_b_OCC / 8) * 4];
    }

    dectnrp_assert(idx == N_b_OCC / 4, "Incorrect number of subcarriers for STF.");
}

void stf_t::fill_y_STF_i(cf_t* y_STF_i_b_x,
                         const int N_b_OCC,
                         const int N_eff_TX,
                         const float scale) {
    // "Define y^(0),beta_r (k) = (-1)^k y^(0),beta (N-k) for all k=1, [...]", however, index value
    // of k=1,...,length() unclear. We assume k starts with 0 and for k=0 the first prefactor is 1.
    auto fliplr_patternp1m1 = [](std::vector<float> inp) {
        const uint32_t sz = inp.size();
        auto out = inp;
        for (uint32_t i = 0; i < sz; ++i) {
            // fliplr
            out[i] = inp[sz - 1 - i];
            // (-1)^k
            out[i] = out[i] * (i % 2 == 0 ? 1.0f : -1.0f);
        }
        return out;
    };

    // output vector polarity +1.0f or -1.0f
    std::vector<float> y_STF_i_b_x_polarity;

    // construction depends on beta
    switch (N_b_OCC) {
        case 56:
            y_STF_i_b_x_polarity = y_b_1;
            break;
        case 112:  // 56*2
            y_STF_i_b_x_polarity = y_b_2;
            break;
        case 224:  // 56*4
            y_STF_i_b_x_polarity = y_b_4;
            break;
        case 448:  // 56*8
        case 672:  // 56*12
        case 896:  // 56*16
            auto y_b_4_r = fliplr_patternp1m1(y_b_4);
            auto y_b_8 = y_b_4;
            y_b_8.insert(y_b_8.end(), y_b_4_r.begin(), y_b_4_r.end());

            if (N_b_OCC == 448) {
                y_STF_i_b_x_polarity = y_b_8;
            } else {
                const auto y_b_8_r = fliplr_patternp1m1(y_b_8);
                auto y_b_16 = y_b_8;
                y_b_16.insert(y_b_16.end(), y_b_8_r.begin(), y_b_8_r.end());

                if (N_b_OCC == 672) {
                    std::vector<float> y_b_12;
                    // N_b_OCC/4 = 672/4 = 12*14
                    for (int i = 0; i <= N_b_OCC / 4 - 1; ++i) {
                        y_b_12.push_back(y_b_16[i + 2 * 14]);
                    }
                    y_STF_i_b_x_polarity = y_b_12;
                } else {
                    y_STF_i_b_x_polarity = y_b_16;
                }
            }
            break;
    }

    // includes exp(j*pi/4)
    const cf_t fac = cf_t{scale, 0.0f} * cf_t{static_cast<float>(std::cos(M_PI / 4.0)),
                                              static_cast<float>(std::sin(M_PI / 4.0))};

    // keep circular rotation as defined in versions prior to TS 103 636-3 V1.4.1 (2023-01)
    uint32_t log2_N_eff_TX = 0;
    switch (N_eff_TX) {
        case 1:
            log2_N_eff_TX = 0;
            break;
        case 2:
            log2_N_eff_TX = 1;
            break;
        case 4:
            log2_N_eff_TX = 2;
            break;
        case 8:
            log2_N_eff_TX = 3;
            break;
    }

    for (int i = 0; i <= N_b_OCC / 4 - 1; ++i) {
        y_STF_i_b_x[i] = cf_t{y_STF_i_b_x_polarity[(i + 2 * log2_N_eff_TX) % (N_b_OCC / 4)], 0.0f};
        y_STF_i_b_x[i] = y_STF_i_b_x[i] * fac;
    }
}

std::vector<cf_t> stf_t::fill_y_STF_filled(const cf_t* y_STF_i_b_x,
                                           const int* k_i,
                                           const int N_b_OCC) {
    // 1+ for zero DC carrier
    std::vector<cf_t> ret(N_b_OCC + 1, cf_t{0.0f, 0.0f});

    // fill STF subcarrier
    for (int32_t i = 0; i < N_b_OCC / 4; ++i) {
        const int32_t index = k_i[i] + N_b_OCC / 2;
        ret[index] = y_STF_i_b_x[i];
    }

    return ret;
}

}  // namespace dectnrp::sp3
