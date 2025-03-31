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

namespace dectnrp::section3 {

/**
 * \brief These are the most basic durations that any DECT NR+ system requires. Some of the values
 * likely do not divide samp_rate with remainder 0. For instance, at 1.728MHz, 100us correspond to
 * 172 samples with 1.728e6/10000=172.8. 1.728e6 is not a multiple of 172.
 */
enum class duration_ec_t : uint32_t {
    us100 = 0,  // likely does not divide samp_rate with remainder zero
    ms001,
    s001,
    slot001,
    subslot_u1_001,
    subslot_u2_001,
    subslot_u4_001,
    subslot_u8_001,
    turn_around_time_us,    // likely does not divide samp_rate with remainder zero
    settling_time_freq_us,  // likely does not divide samp_rate with remainder zero
    settling_time_gain_us,  // likely does not divide samp_rate with remainder zero
    CARDINALITY
};

}  // namespace dectnrp::section3
