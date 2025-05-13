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
#include "dectnrp/radio/hw_friends.hpp"

namespace dectnrp::radio {

class gain_lut_t {
    public:
        struct achievable_power_gain_t {
                float power_dBm;
                float gain_dB;
        };

        [[nodiscard]] achievable_power_gain_t get_achievable_power_gain_tx(const float power_dBm,
                                                                           const float freq_Hz);

        [[nodiscard]] achievable_power_gain_t get_achievable_power_gain_rx(const float power_dBm,
                                                                           const float freq_Hz);

        HW_FRIENDS

    private:
        /// TX and RX use the same frequencies, must be strictly ascending and at least two values
        std::vector<float> freqs_Hz;

        // ##################################################
        // TX

        /// each vector must be at least as long as the counterpart in powers_tx_dBm
        const common::vec2d<float>* gains_tx_dB{nullptr};

        /// must be strictly ascending and at least two values
        const common::vec2d<float>* powers_tx_dBm{nullptr};

        /// step width, typically integer value such as 1dB
        float gains_tx_dB_step;

        // ##################################################
        // RX

        const common::vec2d<float>* gains_rx_dB{nullptr};
        const common::vec2d<float>* powers_rx_dBm{nullptr};
        float gains_rx_dB_step;

        // ##################################################
        // generic

        achievable_power_gain_t get_achievable_power_gain_xx(const common::vec2d<float>& gains,
                                                             const common::vec2d<float>& powers,
                                                             const float gain_dB_step,
                                                             const float power_dBm,
                                                             const float freq_Hz);

        struct interpolation_points_t {
                int32_t idx_left;
                int32_t idx_right;
                float weight_left;
                float weight_right;
        };

        /// find indices of the two closest elements in vec for interpolation of value
        static const interpolation_points_t get_interpolation_points(const std::vector<float>& vec,
                                                                     const float value);

        /// get interpolated value defined by interpolation_points
        static float interpolate(const std::vector<float>& vec,
                                 const interpolation_points_t& interpolation_points);
};

}  // namespace dectnrp::radio
