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

namespace dectnrp::phy {

class resampler_param_t {
    public:
        resampler_param_t() = default;
        explicit resampler_param_t(const uint32_t hw_samp_rate_,
                                   const uint32_t L_,
                                   const uint32_t M_)
            : hw_samp_rate(hw_samp_rate_),
              L(L_),
              M(M_) {};

        /// Kaiser window is defined through stopband attenuation
        static constexpr float PASSBAND_RIPPLE_DONT_CARE = 100.0f;

        /// sample rate of hardware
        uint32_t hw_samp_rate{0};

        /// interpolation
        uint32_t L{0};

        /// decimation
        uint32_t M{0};

        /**
         * \brief Within the SDR, resampling and its implicit filtering is used for TX when
         * transmitting, for STF synchronization and after synchronization right before the FFT.
         */
        enum user_t {
            TX = 0,        // ideally a narrow LPF for TX mask compliance
            SYNC = 1,      // ideally a narrow LFP against out-of-band noise and interference
            RX_SYNCED = 2  // ideally a narrow LFP against out-of-band noise and interference
        };

        /**
         * \brief For LPF definition as part of the resampler, the normalized bandwidth is 0.5Hz.
         * The OFDM bandwidth is approximately 0.5*29/32=0.453125. When oversampling (os) larger 1
         * is used, the OFDM spectrum is squeezed.
         *
         *  |__________|__________|__________|__________|__________|__________|__________|__________|
         *
         *  0Hz     0.0625      0.125      0.1875     0.250      0.3125     0.375     0.4375     0.5
         *           os=8        os=4                  os=2                                     os=1
         */

        /**
         * \brief Filter definition for very wide LPF, so low number of coefficients and thus fast
         * and suitable for large bandwidths. However, the expectable EVM is about 25dB.
         *
         *         os=1   os=2         os=4                     os=8
         *         0.5    0.25         0.125                    0.0625
         */
        static constexpr float f_pass_norm[3][9] = {
            {0.0f, 0.48f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, 0.0f, 0.15f},   // TX
            {0.0f, 0.48f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, 0.0f, 0.15f},   // SYNC
            {0.0f, 0.48f, 0.30f, 0.0f, 0.20f, 0.0f, 0.0f, 0.0f, 0.15f}};  // RX_SYNCED
        static constexpr float f_stop_norm[3][9] = {
            {0.0f, 0.499f, 0.499f, 0.0f, 0.499f, 0.0f, 0.0f, 0.0f, 0.499f},   // TX
            {0.0f, 0.499f, 0.499f, 0.0f, 0.499f, 0.0f, 0.0f, 0.0f, 0.499f},   // SYNC
            {0.0f, 0.499f, 0.499f, 0.0f, 0.499f, 0.0f, 0.0f, 0.0f, 0.499f}};  // RX_SYNCED
        static constexpr float f_stop_att_dB[3][9] = {
            {0.0f, 14.0f, 20.0f, 0.0f, 20.00f, 0.0f, 0.0f, 0.0f, 20.0f},   // TX
            {0.0f, 14.0f, 20.0f, 0.0f, 20.00f, 0.0f, 0.0f, 0.0f, 20.0f},   // SYNC
            {0.0f, 14.0f, 20.0f, 0.0f, 20.00f, 0.0f, 0.0f, 0.0f, 20.0f}};  // RX_SYNCED
};

}  // namespace dectnrp::phy
