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

#include "dectnrp/phy/filter/bessel.hpp"

#include <cmath>

namespace dectnrp::phy::filter {

constexpr uint32_t k_max = 8;

static uint64_t factorial(const uint64_t k) {
    uint64_t ret_val = 1;

    for (uint64_t i = 1; i <= k; ++i) {
        ret_val *= i;
    }

    return ret_val;
}

static double bessel_function_J_or_I_0(const double z, const bool modified) {
    double ret_val = 0.0;

    for (uint32_t k = 0; k <= k_max; ++k) {
        double numerator;

        if (modified) {
            numerator = pow(0.25 * z * z, static_cast<double>(k));
        } else {
            numerator = pow(-0.25 * z * z, static_cast<double>(k));
        }

        double fac = static_cast<double>(factorial(static_cast<uint64_t>(k)));

        ret_val += numerator / (fac * fac);
    }

    return ret_val;
}

float bessel_function_J_0(const float z) { return bessel_function_J_or_I_0(z, false); }

float bessel_function_I_0_modified(const float z) { return bessel_function_J_or_I_0(z, true); }

}  // namespace dectnrp::phy::filter
