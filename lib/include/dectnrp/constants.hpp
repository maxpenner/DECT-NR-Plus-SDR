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

namespace dectnrp::constants {

static constexpr uint32_t N_b_DFT_min_u_b = 64;
static constexpr uint32_t N_b_CP_min_u_b = 8;
static constexpr uint32_t N_b_DFT_CP_min_u_b = N_b_DFT_min_u_b + N_b_CP_min_u_b;

// see Table 4.3-1, ratio between FFT size and cyclic prefix length 1/8=12.5%
static constexpr uint32_t N_b_DFT_to_N_b_CP = N_b_DFT_min_u_b / N_b_CP_min_u_b;

static constexpr uint32_t samp_rate_min_u_b = 1728000;
static constexpr uint32_t subcarrier_spacing_min_u_b = 27000;

static constexpr uint32_t slots_per_10ms = 24;
static constexpr uint32_t slots_per_100ms = 240;
static constexpr uint32_t slots_per_sec = 2400;

static constexpr uint32_t u1_subslots_per_slot = 2;
static constexpr uint32_t u8_subslots_per_slot = 16;

static constexpr uint32_t u1_subslots_per_sec = 4800;
static constexpr uint32_t u2_subslots_per_sec = 9600;
static constexpr uint32_t u4_subslots_per_sec = 19200;
static constexpr uint32_t u8_subslots_per_sec = 38400;

static constexpr uint32_t N_stf_pattern_u1 = 7;
static constexpr uint32_t N_stf_pattern_u248 = 9;
static constexpr uint32_t N_samples_stf_pattern = 16;
static constexpr uint32_t N_samples_stf_u1 = N_stf_pattern_u1 * N_samples_stf_pattern;
static constexpr uint32_t N_samples_stf_u248 = N_stf_pattern_u248 * N_samples_stf_pattern;

// for beta=1, there are 14 occupied STF cells across the full spectrum
static constexpr uint32_t N_STF_cells_b_1 = 14;
static constexpr uint32_t N_STF_cells_separation = 4;
static constexpr uint32_t N_STF_cells_separation_center = 8;

// antenna and MIMO configurations
static constexpr uint32_t N_TS_max = 8;

/**
 * \brief PCC consists of 98 complex subcarriers, each QPSK coded. Thus, the number of bits after
 * channel coding is 196. Before channel coding, PLCF type 1 contains 40 bits and type 2 contains 80
 * bits. During channel coding, 16 bits CRC are added (see 7.5.2.1 in part 3).
 *
 * Thus, type 1 has a coding rate of (40+16)/196=0.28571.
 * Thus, type 2 has a coding rate of (80+16)/196=0.48979.
 *
 * As far as FEC is concerned, we could also use 40, 48, 56, 64, 72, 80 and higher values
 * (Table 6.1.4.2.3-1 in part 3). Also, we could define more types (3, 4 etc.), but at the receiver
 * we would have to test against each type blindly.
 */
static constexpr uint32_t plcf_type_1_byte = 5;
static constexpr uint32_t plcf_type_1_bit = plcf_type_1_byte * 8;
static constexpr uint32_t plcf_type_2_byte = 10;
static constexpr uint32_t plcf_type_2_bit = plcf_type_2_byte * 8;

static constexpr uint32_t pcc_bits = 196;
static constexpr uint32_t pcc_cells = 98;  // cells = complex subcarriers

static constexpr uint32_t rv_max = 3;
static constexpr uint32_t rv_unwrapped_max = 7;

}  // namespace dectnrp::constants
