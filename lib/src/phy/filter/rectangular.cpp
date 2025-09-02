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

#include "dectnrp/phy/filter/rectangular.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::filter {

float sinc(const float n) { return (n == 0.0f) ? 1.0f : std::sin(M_PI * n) / (M_PI * n); }

std::vector<float> rectangular_window(const float f_cutoff, const uint32_t N) {
    dectnrp_assert(0.0f < f_cutoff && f_cutoff < 0.5f,
                   "Cutoff frequency must be normalized and between 0 and 0.5.");

    std::vector<float> filter_coefficients(N);

    for (uint32_t n = 0; n < N; ++n) {
        float N_f = static_cast<float>(N);
        float n_f = static_cast<float>(n);

        filter_coefficients[n] =
            2.0f * f_cutoff * sinc(2.0f * f_cutoff * (n_f - (N_f - 1.0f) / 2.0f));
    }

    return filter_coefficients;
}

}  // namespace dectnrp::phy::filter
