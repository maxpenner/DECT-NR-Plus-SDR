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

#include "dectnrp/phy/filter/raised_cosine.hpp"

#include <cmath>
#include <numbers>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::filter {

float raised_cosine(const uint32_t n, const uint32_t N) {
    if (N == 0) {
        return 1.0f;
    }

    dectnrp_assert(n < N, "n too large");

    const float n_f = static_cast<float>(n);
    const float N_f = static_cast<float>(N);

    /* If n_f = 0, then A becomes cos(-pi/2) = 0.
     * If n_f = N_f, then A becomes cos(0) = 1.
     */
    const float A = std::cos((n_f - N_f) / N_f * std::numbers::pi_v<float> * 0.5f);

    return A * A;
}

std::vector<float> raised_cosine_window_rising_edge(const uint32_t N) {
    std::vector<float> filter_coefficients(N);

    for (uint32_t n = 0; n < N; ++n) {
        filter_coefficients.at(n) = raised_cosine(n, N);
    }

    return filter_coefficients;
}

}  // namespace dectnrp::phy::filter
