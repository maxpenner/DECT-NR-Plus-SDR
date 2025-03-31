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

#include <array>
#include <cstdint>
#include <vector>

namespace dectnrp::common {

template <typename T, std::size_t A, std::size_t B>
using array2d = std::array<std::array<T, B>, A>;

template <typename T, std::size_t A, std::size_t B, std::size_t C>
using array3d = std::array<array2d<T, B, C>, A>;

template <typename T, std::size_t A, std::size_t B, std::size_t C, std::size_t D>
using array4d = std::array<array3d<T, B, C, D>, A>;

template <typename T>
using vec2d = std::vector<std::vector<T>>;

template <typename T>
using vec3d = std::vector<vec2d<T>>;

template <typename T>
using vec4d = std::vector<vec3d<T>>;

}  // namespace dectnrp::common
