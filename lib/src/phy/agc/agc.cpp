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

#include "dectnrp/phy/agc/agc.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"

namespace dectnrp::phy::agc {

agc_t::agc_t(const agc_config_t agc_config_)
    : agc_config(agc_config_),
      roundrobin(roundrobin_t(agc_config.nof_antennas)) {
    dectnrp_assert(0 < agc_config.nof_antennas, "too small");
    dectnrp_assert(agc_config.nof_antennas <= limits::dectnrp_max_nof_antennas, "too large");

    dectnrp_assert(0.5f <= agc_config.gain_step_dB_multiple, "too small");
    dectnrp_assert(agc_config.gain_step_dB_multiple <= 5.0f, "too large");

    dectnrp_assert(agc_config.gain_step_dB_max >= agc_config.gain_step_dB_min, "out of order");

    dectnrp_assert(
        is_positive_multiple(agc_config.gain_step_dB_max, agc_config.gain_step_dB_multiple),
        "not a multiple");

    dectnrp_assert(
        is_positive_multiple(agc_config.gain_step_dB_min, agc_config.gain_step_dB_multiple),
        "not a multiple");
}

float agc_t::quantize_and_limit_gain_step_dB(float arbitrary_gain_step_dB) {
    // make it a multiple of gain_step_dB_multiple
    const float quantized_gain_step_dB =
        std::round(arbitrary_gain_step_dB / agc_config.gain_step_dB_multiple) *
        agc_config.gain_step_dB_multiple;

    // in dead band?
    if (-agc_config.gain_step_dB_min < quantized_gain_step_dB &&
        quantized_gain_step_dB < agc_config.gain_step_dB_min) {
        return 0.0f;
    }

    // too large?
    if (agc_config.gain_step_dB_max < quantized_gain_step_dB) {
        return agc_config.gain_step_dB_max;
    }

    // too small?
    if (quantized_gain_step_dB < -agc_config.gain_step_dB_max) {
        return -agc_config.gain_step_dB_max;
    }

    return quantized_gain_step_dB;
}

common::ant_t agc_t::quantize_and_limit_gain_step_dB(const common::ant_t& arbitrary_gain_step_dB) {
    common::ant_t ret(arbitrary_gain_step_dB.get_nof_antennas());

    for (size_t i = 0; i < agc_config.nof_antennas; ++i) {
        ret.at(i) = quantize_and_limit_gain_step_dB(arbitrary_gain_step_dB.at(i));
    }

    return ret;
}

bool agc_t::is_positive_multiple(const float inp, const float multiple) {
    dectnrp_assert(inp > 0.0f, "inp must be positive");
    dectnrp_assert(multiple > 0.0f, "multiple must be positive");

    return (inp / multiple) == std::round(inp / multiple);
}

}  // namespace dectnrp::phy::agc
