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

#include <cstdint>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common::adt {

template <typename T>
class expiring_t {
    public:
        expiring_t() = default;
        expiring_t(const T val_, const int64_t time_64_) noexcept
            : val(val_),
              time_64(time_64_) {};

        bool is_valid(const int64_t latest_64) const noexcept { return latest_64 <= time_64; };
        bool is_expired(const int64_t latest_64) const noexcept { return time_64 < latest_64; };

        constexpr const T* operator->() const noexcept { return &val; }
        constexpr T* operator->() noexcept { return &val; }

        constexpr const T& operator*() const& noexcept { return val; }
        constexpr T& operator*() & noexcept { return val; }

    private:
        T val{};
        int64_t time_64{UNDEFINED_EARLY_64};
};

}  // namespace dectnrp::common::adt
