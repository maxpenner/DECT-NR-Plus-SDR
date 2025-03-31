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

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <limits>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common::adt {

template <std::unsigned_integral T>
constexpr T ceil_divide_integer(T x, T y) {
    return (x + y - 1) / y;
}

template <std::integral T>
constexpr T round_integer(T x, T y) {
    const T offset = (((x < 0) == (y < 0)) ? y : -y) / 2;
    return (x + offset) / y;
}

template <std::signed_integral T, std::signed_integral M>
constexpr T multiple_leq(T x, M m) {
    dectnrp_assert(m >= M{1}, "multiple must be at least 1");
    return (x / m) * m;
}

template <std::signed_integral T, std::signed_integral M>
constexpr T multiple_geq(T x, M m) {
    const T leqm = multiple_leq(x, m);
    return leqm < x ? leqm + m : leqm;
}

/// checks whether a value is present in a container
template <typename C, typename V>
constexpr bool contains(const C& c, const V v) {
    return std::find(c.begin(), c.end(), v) != c.end();
}

/**
 * \brief most fields in protocol data units cover a small range of possible positive values, this
 * is the default undefined value
 */
static constexpr uint32_t UNDEFINED_NUMERIC_32 = std::numeric_limits<uint32_t>::max();

/**
 * \brief time very far in the past, system time always starts at 0
 */
static constexpr int64_t UNDEFINED_EARLY_64 = std::numeric_limits<int64_t>::min() / int64_t{8};

}  // namespace dectnrp::common::adt
