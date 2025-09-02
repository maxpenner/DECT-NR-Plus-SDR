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

namespace dectnrp::common::adt {

template <std::floating_point T>
T mag2db(const T t) {
    return T{20.0} * std::log10(t);
}

template <std::floating_point T>
T db2mag(const T t) {
    return std::pow(T{10.0}, t / T{20.0});
}

template <std::floating_point T>
T pow2db(const T t) {
    return T{10.0} * std::log10(t);
}

template <std::floating_point T>
T db2pow(const T t) {
    return std::pow(T{10.0}, t / T{10.0});
}

}  // namespace dectnrp::common::adt
