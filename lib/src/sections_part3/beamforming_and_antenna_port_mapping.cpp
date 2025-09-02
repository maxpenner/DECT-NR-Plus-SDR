/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/sections_part3/beamforming_and_antenna_port_mapping.hpp"

#include <algorithm>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp3 {

W_t::W_t() {
    // initialize 7 rows
    for (uint32_t i = 0; i < 7; ++i) {
        W.push_back(common::vec2d<cf_t>());
    }

    // SISO
    W[0].push_back(std::vector<cf_t>{cf_t{1.0f, 0.0f}});

    // N_TS=1 N_TX=2
    for (const auto& elem : W_1) {
        W[1].push_back(convert_to_cf32(elem));
    }

    // N_TS=1 N_TX=4
    for (const auto& elem : W_2) {
        W[2].push_back(convert_to_cf32(elem));
    }

    // N_TS=2 N_TX=2
    for (const auto& elem : W_3) {
        W[3].push_back(convert_to_cf32(elem));
    }

    // N_TS=2 N_TX=4
    for (const auto& elem : W_4) {
        W[4].push_back(convert_to_cf32(elem));
    }

    // N_TS=4 N_TX=4
    for (const auto& elem : W_5) {
        W[5].push_back(convert_to_cf32(elem));
    }

    // N_TS=8 N_TX=8
    for (const auto& elem : W_6) {
        W[6].push_back(convert_to_cf32(elem));
    }

    dectnrp_assert(W.size() == 7, "incorrect number of rows");
    dectnrp_assert(W[0].size() == 1, "incorrect size");
    dectnrp_assert(W[1].size() == 6, "incorrect size");
    dectnrp_assert(W[2].size() == 28, "incorrect size");
    dectnrp_assert(W[3].size() == 3, "incorrect size");
    dectnrp_assert(W[4].size() == 22, "incorrect size");
    dectnrp_assert(W[5].size() == 5, "incorrect size");
    dectnrp_assert(W[6].size() == 1, "incorrect size");
    dectnrp_assert(
        std::all_of(W[0].begin(), W[0].end(), [](const auto& elem) { return elem.size() == 1; }),
        "incorrect size");
    dectnrp_assert(
        std::all_of(W[1].begin(), W[1].end(), [](const auto& elem) { return elem.size() == 2; }),
        "incorrect size");
    dectnrp_assert(
        std::all_of(W[2].begin(), W[2].end(), [](const auto& elem) { return elem.size() == 4; }),
        "incorrect size");
    dectnrp_assert(
        std::all_of(W[3].begin(), W[3].end(), [](const auto& elem) { return elem.size() == 4; }),
        "incorrect size");
    dectnrp_assert(
        std::all_of(W[4].begin(), W[4].end(), [](const auto& elem) { return elem.size() == 8; }),
        "incorrect size");
    dectnrp_assert(
        std::all_of(W[5].begin(), W[5].end(), [](const auto& elem) { return elem.size() == 16; }),
        "incorrect size");
    dectnrp_assert(
        std::all_of(W[6].begin(), W[6].end(), [](const auto& elem) { return elem.size() == 64; }),
        "incorrect size");

    // initialize 7 rows
    for (uint32_t i = 0; i < 7; ++i) {
        scaling_factor.push_back(std::vector<float>());
    }

    // SISO
    scaling_factor[0].push_back(1.0f);

    // N_TS=1 N_TX=2
    for (const auto& elem : W_1) {
        scaling_factor[1].push_back(get_W_scaling_factor(elem));
    }

    // N_TS=1 N_TX=4
    for (const auto& elem : W_2) {
        scaling_factor[2].push_back(get_W_scaling_factor(elem));
    }

    // N_TS=2 N_TX=2
    for (const auto& elem : W_3) {
        scaling_factor[3].push_back(get_W_scaling_factor(elem));
    }

    // N_TS=2 N_TX=4
    for (const auto& elem : W_4) {
        scaling_factor[4].push_back(get_W_scaling_factor(elem));
    }

    // N_TS=4 N_TX=4
    for (const auto& elem : W_5) {
        scaling_factor[5].push_back(get_W_scaling_factor(elem));
    }

    // N_TS=8 N_TX=8
    for (const auto& elem : W_6) {
        scaling_factor[6].push_back(get_W_scaling_factor(elem));
    }

    dectnrp_assert(scaling_factor.size() == 7, "incorrect number of rows");
    dectnrp_assert(scaling_factor[0].size() == 1, "incorrect size");
    dectnrp_assert(scaling_factor[1].size() == 6, "incorrect size");
    dectnrp_assert(scaling_factor[2].size() == 28, "incorrect size");
    dectnrp_assert(scaling_factor[3].size() == 3, "incorrect size");
    dectnrp_assert(scaling_factor[4].size() == 22, "incorrect size");
    dectnrp_assert(scaling_factor[5].size() == 5, "incorrect size");
    dectnrp_assert(scaling_factor[6].size() == 1, "incorrect size");

    // initialize 7 rows
    for (uint32_t i = 0; i < 7; ++i) {
        scaling_factor_optimal_DAC.push_back(std::vector<float>());
    }

    // SISO
    scaling_factor_optimal_DAC[0].push_back(1.0f);

    // N_TS=1 N_TX=2
    for (uint32_t i = 0; i < 6; ++i) {
        scaling_factor_optimal_DAC[1].push_back(1.0f);
    }

    // N_TS=1 N_TX=4
    for (uint32_t i = 0; i < 28; ++i) {
        scaling_factor_optimal_DAC[2].push_back(1.0f);
    }

    // N_TS=2 N_TX=2
    scaling_factor_optimal_DAC[3] =
        std::vector<float>{1.0f, 1.0f / std::sqrt(2.0f), 1.0f / std::sqrt(2.0f)};

    // N_TS=2 N_TX=4
    for (uint32_t i = 0; i < 14; ++i) {
        scaling_factor_optimal_DAC[4].push_back(1.0f);
    }
    for (uint32_t i = 0; i < 8; ++i) {
        scaling_factor_optimal_DAC[4].push_back(1.0f / std::sqrt(2.0f));
    }

    // N_TS=4 N_TX=4
    scaling_factor_optimal_DAC[5] = std::vector<float>{1.0f,
                                                       1.0f / std::sqrt(2.0f),
                                                       1.0f / std::sqrt(2.0f),
                                                       1.0f / std::sqrt(4.0f),
                                                       1.0f / std::sqrt(4.0f)};

    // N_TS=8 N_TX=8
    scaling_factor_optimal_DAC[6].push_back(1.0f);

    dectnrp_assert(scaling_factor_optimal_DAC.size() == 7, "incorrect number of rows");
    dectnrp_assert(scaling_factor_optimal_DAC[0].size() == 1, "incorrect size");
    dectnrp_assert(scaling_factor_optimal_DAC[1].size() == 6, "incorrect size");
    dectnrp_assert(scaling_factor_optimal_DAC[2].size() == 28, "incorrect size");
    dectnrp_assert(scaling_factor_optimal_DAC[3].size() == 3, "incorrect size");
    dectnrp_assert(scaling_factor_optimal_DAC[4].size() == 22, "incorrect size");
    dectnrp_assert(scaling_factor_optimal_DAC[5].size() == 5, "incorrect size");
    dectnrp_assert(scaling_factor_optimal_DAC[6].size() == 1, "incorrect size");
}

uint32_t W_t::clamp_W(const uint32_t N_TS, const uint32_t N_TX, const uint32_t codebook_idx) {
    return std::min(codebook_idx,
                    static_cast<uint32_t>(N_TS_N_TX_codebook_index_max.at(N_TS).at(N_TX)));
}

uint32_t W_t::clamp_W(const tmmode::tm_mode_t& tm_mode, const uint32_t codebook_idx) {
    return clamp_W(tm_mode.N_TS, tm_mode.N_TX, codebook_idx);
}

const std::vector<cf_t>& W_t::get_W(const uint32_t N_TS,
                                    const uint32_t N_TX,
                                    const uint32_t codebook_idx) const {
    // return W[N_TS_N_TX_idx[N_TS][N_TX]][codebook_idx];
    return W.at(N_TS_N_TX_idx.at(N_TS).at(N_TX)).at(codebook_idx);
}

const common::vec2d<cf_t>& W_t::get_W(const uint32_t N_TS, const uint32_t N_TX) const {
    // return W[N_TS_N_TX_idx[N_TS][N_TX]];
    return W.at(N_TS_N_TX_idx.at(N_TS).at(N_TX));
}

float W_t::get_scaling_factor(const uint32_t N_TS,
                              const uint32_t N_TX,
                              const uint32_t codebook_idx) const {
    return scaling_factor.at(N_TS_N_TX_idx.at(N_TS).at(N_TX)).at(codebook_idx);
}

const std::vector<float>& W_t::get_scaling_factor(const uint32_t N_TS, const uint32_t N_TX) const {
    return scaling_factor.at(N_TS_N_TX_idx.at(N_TS).at(N_TX));
}

float W_t::get_scaling_factor_optimal_DAC(const uint32_t N_TS,
                                          const uint32_t N_TX,
                                          const uint32_t codebook_idx) const {
    return scaling_factor_optimal_DAC.at(N_TS_N_TX_idx.at(N_TS).at(N_TX)).at(codebook_idx);
}

const std::vector<float>& W_t::get_scaling_factor_optimal_DAC(const uint32_t N_TS,
                                                              const uint32_t N_TX) const {
    return scaling_factor_optimal_DAC.at(N_TS_N_TX_idx.at(N_TS).at(N_TX));
}

uint32_t W_t::get_codebook_index_max(const uint32_t N_TS, const uint32_t N_TX) {
    return N_TS_N_TX_codebook_index_max.at(N_TS).at(N_TX);
}

uint32_t W_t::get_codebook_index_nonzero(const uint32_t N_TS, const uint32_t N_TX) {
    // according to the TS the default index of the identity beamforming matrix is 0, here we can
    // use other values to increase the overall output power

    // return N_TS_N_TX_codebook_index_nonzero[N_TS][N_TX];
    return N_TS_N_TX_codebook_index_nonzero.at(N_TS).at(N_TX);
}

const common::vec2d<int8_t> W_t::W_1 = {{1, 0}, {0, 1}, {1, 1}, {1, -1}, {1, j}, {1, -j}};

const common::vec2d<int8_t> W_t::W_2 = {
    {1, 0, 0, 0},   {0, 1, 0, 0},  {0, 0, 1, 0},   {0, 0, 0, 1},   {1, 0, 1, 0},   {1, 0, -1, 0},
    {1, 0, j, 0},   {1, 0, -j, 0}, {0, 1, 0, 1},   {0, 1, 0, -1},  {0, 1, 0, j},   {0, 1, 0, -j},
    {1, 1, 1, 1},   {1, 1, j, j},  {1, 1, -1, -1}, {1, 1, -j, -j}, {1, j, 1, j},   {1, j, j, -1},
    {1, j, -1, -j}, {1, j, -j, 1}, {1, -1, 1, -1}, {1, -1, j, -j}, {1, -1, -1, 1}, {1, -1, -j, j},
    {1, -j, 1, -j}, {1, -j, j, 1}, {1, -j, -1, j}, {1, -j, -j, -1}};

const common::vec2d<int8_t> W_t::W_3 = {{1, 0, 0, 1}, {1, 1, 1, -1}, {1, 1, j, -j}};

const common::vec2d<int8_t> W_t::W_4 = {
    {1, 0, 0, 1, 0, 0, 0, 0},     {1, 0, 0, 0, 0, 1, 0, 0},     {1, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 1, 0, 0, 1, 0, 0},     {0, 0, 1, 0, 0, 0, 0, 1},     {0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 1, 1, 0, 0, -j},    {1, 0, 0, 1, 1, 0, 0, j},     {1, 0, 0, 1, -j, 0, 0, 1},
    {1, 0, 0, 1, -j, 0, 0, -1},   {1, 0, 0, 1, -1, 0, 0, -j},   {1, 0, 0, 1, -1, 0, 0, j},
    {1, 0, 0, 1, j, 0, 0, 1},     {1, 0, 0, 1, j, 0, 0, -1},    {1, 1, 1, 1, 1, -1, 1, -1},
    {1, 1, 1, 1, j, -j, j, -j},   {1, 1, j, j, 1, -1, j, -j},   {1, 1, j, j, j, -j, -1, 1},
    {1, 1, -1, -1, 1, -1, -1, 1}, {1, 1, -1, -1, j, -j, -j, j}, {1, 1, -j, -j, 1, -1, -j, j},
    {1, 1, -j, -j, j, -j, 1, -1}};

const common::vec2d<int8_t> W_t::W_5 = {
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 1, 0, 0, 0, 0, 1, 1, 1, -1, 0, 0, 0, 0, 1, -1},
    {1, 1, 0, 0, 0, 0, 1, 1, j, -j, 0, 0, 0, 0, j, -j},
    {1, 1, 1, 1, 1, -1, 1, -1, 1, 1, -1, -1, 1, -1, -1, 1},
    {1, 1, 1, 1, 1, -1, 1, -1, j, j, -j, -j, j, -j, -j, j},
};

const common::vec2d<int8_t> W_t::W_6 = {{1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                                         0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                                         0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
                                         0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1}};

std::vector<cf_t> W_t::convert_to_cf32(const std::vector<int8_t> in) {
    std::vector<cf_t> ret;

    // convert from int to float
    for (uint32_t i = 0; i < in.size(); ++i) {
        if (in[i] == 1) {
            ret.push_back(cf_t{1.0f, 0.0f});
        } else if (in[i] == -1) {
            ret.push_back(cf_t{-1.0f, 0.0f});
        } else if (in[i] == j) {
            ret.push_back(cf_t{0.0f, 1.0f});
        } else if (in[i] == -j) {
            ret.push_back(cf_t{0.0f, -1.0f});
        } else if (in[i] == 0) {
            ret.push_back(cf_t{0.0f, 0.0f});
        } else {
            dectnrp_assert_failure("undefined value");
        }
    }

    return ret;
}

float W_t::get_W_scaling_factor(const std::vector<int8_t> in) {
    // each matrix has a scaling factor to preserve an average power of one
    float scale_power = 0.0f;

    // the scaling factor is the sqrtf of non-zero values in each matrix W
    for (uint32_t i = 0; i < in.size(); ++i) {
        if (in[i] != 0) {
            scale_power += 1.0f;
        }
    }

    return 1.0f / std::sqrt(scale_power);
}

}  // namespace dectnrp::sp3
