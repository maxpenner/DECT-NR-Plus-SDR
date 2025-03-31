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

#include "dectnrp/phy/filter/kaiser.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/filter/bessel.hpp"
#include "dectnrp/phy/filter/rectangular.hpp"

namespace dectnrp::phy::filter {

// sources (notation from last two sources)
// https://de.mathworks.com/help/signal/ref/kaiser.html
// https://de.mathworks.com/help/signal/ref/kaiserord.html
// https://tomroelandts.com/articles/how-to-create-a-simple-low-pass-filter
// https://tomroelandts.com/articles/how-to-create-a-configurable-filter-using-a-kaiser-window
std::vector<float> kaiser(const float PassbandFrequency,
                          const float StopbandFrequency,
                          const float PassbandRipple_dB,
                          const float StopbandAttenuation_dB,
                          const float SampleRate,
                          const bool N_force_odd) {
    /* The filter coefficients are designed by using A, which is the stopband attenuation. The value
     * of A can be defined by using argument StopbandAttenuation_dB, or it can be derived from
     * PassbandRipple_dB (the smaller the passband ripple, the larger the stopband attenuation).
     * Therefore, we have to check which is is the stricter definition of A (the larger value of A).
     * delta is the allowed ripple in pass band.
     */
    // delta defined through StopbandAttenuation_dB
    float delta_option_0 = std::pow(10.0f, -StopbandAttenuation_dB / 20.0f);
    // delta defined through PassbandRipple_dB
    float delta_option_1 = std::pow(10.0f, PassbandRipple_dB / 20.0f) - 1.0f;
    float delta = std::min(delta_option_0, delta_option_1);
    // this is the actual stopband attenuation we have to achieve
    float A = -20.0f * std::log10(delta);

    // with the value of A given, we can now derive the Kaiser parameter beta
    float beta = 0.0f;
    if (A > 50.0f) {
        beta = 0.1102f * (A - 8.7f);
    } else if (21.0f <= A && A <= 50.0f) {
        beta = 0.5842f * std::pow((A - 21.0f), 0.4f) + 0.07886 * (A - 21.0f);
    } else if (A < 21.0f) {
        beta = 0.0f;
    }

    // normalize frequencies
    float PassbandFrequency_norm = PassbandFrequency / SampleRate;
    float StopbandFrequency_norm = StopbandFrequency / SampleRate;

    dectnrp_assert(PassbandFrequency_norm < StopbandFrequency_norm,
                   "Passband must be smaller than stopband.");

    // transition bandwidth
    float b = StopbandFrequency_norm - PassbandFrequency_norm;

    // with the value of A and b given, we can now derive the filter order
    float filter_order = (A - 7.95f) / (2.285f * 2.0f * M_PI * b);

    // number of taps
    uint32_t N = static_cast<uint32_t>(std::ceil(filter_order + 1.0f));

    // make sure we have an odd number of taps to avoid fractional delay
    if (N_force_odd) {
        if (N % 2 == 0) {
            N += 1;
        }
    }

    // Kaiser windowing coefficients in time domain
    std::vector<float> w_n(N);
    for (uint32_t n = 0; n < N; ++n) {
        float N_f = static_cast<float>(N);
        float n_f = static_cast<float>(n);

        float w_n_arg = beta * std::sqrt(1.0f - std::pow(2.0f * n_f / (N_f - 1.0f) - 1.0f, 2.0f));

        w_n[n] = bessel_function_I_0_modified(w_n_arg) / bessel_function_I_0_modified(beta);
    }

    // cutoff frequency lies in the center between passband and stopband
    float f_cutoff = PassbandFrequency_norm + b / 2.0f;

    // unwindowed sinc filter coefficients
    const std::vector<float> rectangular_unkaisered = rectangular_window(f_cutoff, N);

    // Kaiser filter coefficients before normalization
    float norm = 0.0f;
    std::vector<float> rectangular_kaisered(N);
    for (uint32_t n = 0; n < N; ++n) {
        rectangular_kaisered[n] = w_n[n] * rectangular_unkaisered[n];
        norm += rectangular_kaisered[n];
    }

    // normalize and set output
    std::vector<float> filter_coefficients(N);
    for (uint32_t n = 0; n < N; ++n) {
        filter_coefficients[n] = rectangular_kaisered[n] / norm;
    }

    return filter_coefficients;
}

}  // namespace dectnrp::phy::filter
