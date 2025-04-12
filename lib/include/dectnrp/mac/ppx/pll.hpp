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
#include <vector>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/sections_part3/derivative/duration.hpp"

namespace dectnrp::mac::ppx {

class pll_t {
    public:
        pll_t() = default;
        pll_t(const section3::duration_t beacon_period_);

        void provide_beacon_time(const int64_t beacon_time_64);

        template <typename T>
            requires(std::is_arithmetic_v<T>)
        T get_warped(const T length) const {
            const float A = std::round(static_cast<float>(length) * warp_factor);
            return static_cast<T>(A);
        }

    private:
        section3::duration_t beacon_period;

        int64_t new_value_64;
        int64_t separation_min_64;
        int64_t separation_max_64;

        std::vector<int64_t> beacon_time_vec;
        std::size_t idx{};

        /// observed time base warping between TX and RX, value will be very close to 1.0f
        float warp_factor{1.0f};

        std::size_t prev_idx() const { return idx == 0 ? beacon_time_vec.size() - 1 : idx - 1; };
        std::size_t next_idx() const { return idx == beacon_time_vec.size() - 1 ? 0 : idx + 1; };
};

}  // namespace dectnrp::mac::ppx
