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

#pragma once

#include <cmath>
#include <concepts>
#include <numbers>

namespace dectnrp::common::adt {

template <std::floating_point T, std::unsigned_integral U>
T get_sample2sample_phase_inc(const T freq_shift, const U hw_samp_rate) {
    return T{2.0} * std::numbers::pi_v<T> * freq_shift / static_cast<T>(hw_samp_rate);
}

}  // namespace dectnrp::common::adt
