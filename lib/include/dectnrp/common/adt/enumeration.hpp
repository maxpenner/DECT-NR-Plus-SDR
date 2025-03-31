/*
 * Copyright 2023-2024 Maxim Penner, Alexander Poets
 * Copyright 2025-2025 Maxim Penner
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
#include <utility>

namespace dectnrp::common::adt {

template <typename E>
    requires requires {
        requires std::is_scoped_enum_v<E>;
        E::not_defined;
        E::upper;
    }
E from_coded_value(const std::underlying_type_t<E> value) {
    if constexpr (requires { E::lower; }) {
        return std::to_underlying(E::lower) < value && value < std::to_underlying(E::upper)
                   ? static_cast<E>(value)
                   : E::not_defined;
    } else {
        return value < std::to_underlying(E::upper) ? static_cast<E>(value) : E::not_defined;
    }
}

template <typename E>
    requires requires {
        requires std::is_scoped_enum_v<E>;
        E::not_defined;
    }
bool is_valid(const E value) {
    if (value == E::not_defined) {
        return false;
    }

    if constexpr (requires { E::lower; }) {
        if (value == E::lower) {
            return false;
        }
    }

    if constexpr (requires { E::upper; }) {
        if (value == E::upper) {
            return false;
        }
    }

    return true;
}

}  // namespace dectnrp::common::adt
