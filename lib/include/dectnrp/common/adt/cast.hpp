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

#include <concepts>
#include <cstdint>
#include <limits>

namespace dectnrp::common::adt::cast {

template <std::signed_integral S>
static constexpr double get_scale_int() {
    if constexpr (std::is_same_v<S, int8_t>) {
        return 1.0e1;
    } else if constexpr (std::is_same_v<S, int16_t>) {
        return 1.0e3;
    } else if constexpr (std::is_same_v<S, int32_t>) {
        return 1.0e5;
    } else {
        static_assert(std::is_same_v<S, int64_t>, "unknown signed integral");
    }

    return 1.0e7;
}

template <std::floating_point F = float, std::signed_integral S = int32_t>
static constexpr S float_to_int(const F input, const F scale = F(get_scale_int<S>())) {
    constexpr S min = std::numeric_limits<S>::min();
    constexpr S max = std::numeric_limits<S>::max();

    constexpr F minf = static_cast<float>(min);
    constexpr F maxf = static_cast<float>(max);

    const F input_scaled = input * scale;

    if (input_scaled <= minf) {
        return min;
    }

    if (maxf <= input_scaled) {
        return max;
    }

    return static_cast<S>(input_scaled);
}

}  // namespace dectnrp::common::adt::cast
