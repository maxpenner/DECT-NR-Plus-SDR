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

#include "dectnrp/radio/gain_lut.hpp"

#include <cmath>

namespace dectnrp::radio {

gain_lut_t::achievable_power_gain_t gain_lut_t::get_achievable_power_gain_tx(const float power_dBm,
                                                                             const float freq_Hz) {
    return get_achievable_power_gain_xx(
        *gains_tx_dB, *powers_tx_dBm, gains_tx_dB_step, power_dBm, freq_Hz);
}

gain_lut_t::achievable_power_gain_t gain_lut_t::get_achievable_power_gain_rx(const float power_dBm,
                                                                             const float freq_Hz) {
    return get_achievable_power_gain_xx(
        *gains_rx_dB, *powers_rx_dBm, gains_rx_dB_step, power_dBm, freq_Hz);
}

gain_lut_t::achievable_power_gain_t gain_lut_t::get_achievable_power_gain_xx(
    const common::vec2d<float>& gains,
    const common::vec2d<float>& powers,
    const float gain_dB_step,
    const float power_dBm,
    const float freq_Hz) {
    // find indices of the two optimal frequencies for interpolation
    const auto ip_freq = get_interpolation_points(freqs_Hz, freq_Hz);

    // find indices of the two optimal power values for interpolation at left and right frequency
    const auto ip_power_l = get_interpolation_points(powers[ip_freq.idx_left], power_dBm);
    const auto ip_power_r = get_interpolation_points(powers[ip_freq.idx_right], power_dBm);

    // interpolate left and right power
    const float power_l = interpolate(powers[ip_freq.idx_left], ip_power_l);
    const float power_r = interpolate(powers[ip_freq.idx_right], ip_power_r);

    // interpolate left and right gain at the same indices and with the same weights as the powers
    const float gain_l = interpolate(gains[ip_freq.idx_left], ip_power_l);
    const float gain_r = interpolate(gains[ip_freq.idx_right], ip_power_r);

    achievable_power_gain_t ret;

    // interpolate across left and right frequencies
    ret.power_dBm = power_l * ip_freq.weight_left + power_r * ip_freq.weight_right;
    ret.gain_dB = gain_l * ip_freq.weight_left + gain_r * ip_freq.weight_right;

    // gain must be a multiple of step
    const float gain_dB_stepped = std::round(ret.gain_dB / gain_dB_step) * gain_dB_step;

    // adjust power accordingly
    ret.power_dBm += gain_dB_stepped - ret.gain_dB;

    // overwrite gain
    ret.gain_dB = gain_dB_stepped;

    return ret;
}

const gain_lut_t::interpolation_points_t gain_lut_t::get_interpolation_points(
    const std::vector<float>& vec, const float value) {
    // limit to our range
    float value_limited = std::max(vec.front(), value);
    value_limited = std::min(vec.back(), value_limited);

    interpolation_points_t ret;

    // find first element in vec larger than value or choose last element
    ret.idx_right = vec.size() - 1;
    for (uint32_t i = 0; i < vec.size(); ++i) {
        if (value_limited < vec[i]) {
            ret.idx_right = i;
            break;
        }
    }

    // always choose the neighbouring frequency
    ret.idx_left = ret.idx_right - 1;

    // determine weights
    const float diff_left = value_limited - vec[ret.idx_left];
    const float diff_right = vec[ret.idx_right] - value_limited;

    if (diff_left + diff_right > 0.0) {
        ret.weight_left = diff_right / (diff_left + diff_right);
    } else {
        ret.weight_left = 0;
    }

    ret.weight_right = 1.0 - ret.weight_left;

    return ret;
}

float gain_lut_t::interpolate(const std::vector<float>& vec,
                              const interpolation_points_t& interpolation_points) {
    return vec[interpolation_points.idx_left] * interpolation_points.weight_left +
           vec[interpolation_points.idx_right] * interpolation_points.weight_right;
}

}  // namespace dectnrp::radio
