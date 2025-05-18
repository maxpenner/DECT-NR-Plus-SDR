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

#include "dectnrp/common/adt/ema.hpp"
#include "dectnrp/sections_part3/derivative/duration.hpp"

namespace dectnrp::mac {

class pll_t {
    public:
        pll_t() = default;
        pll_t(const section3::duration_t beacon_period_);

        void provide_beacon_time(const int64_t beacon_time_64);

        /// negative if invalid
        int64_t get_beacon_time_last_known() const { return beacon_time_vec.at(prev_idx()); };

        /// negative if invalid
        int64_t get_beacon_time_oldest_known() const { return beacon_time_vec.at(next_idx()); };

        void reset();

        template <typename T>
            requires(std::is_arithmetic_v<T>)
        T get_warped(const T length) const {
            const double A = std::round(static_cast<double>(length) * warp_factor_ema.get_val());
            return static_cast<T>(A);
        }

        double convert_warp_factor_to_ppm() const;

    private:
        section3::duration_t beacon_period;

        /// minimum time distance between two beacons to accept the latter one
        int64_t dist_min_accept_64;

        /// minimum time distance between two beacons to measure the warping
        int64_t dist_min_64;

        /// maximum time distance between two beacons to measure the warping
        int64_t dist_max_64;

        /// collection of past beacons used to measure the warping between the time bases
        std::vector<int64_t> beacon_time_vec;
        std::size_t idx{};

        common::adt::ema_t<double, double> warp_factor_ema;

        std::size_t prev_idx() const { return idx == 0 ? beacon_time_vec.size() - 1 : idx - 1; };
        std::size_t next_idx() const { return idx == beacon_time_vec.size() - 1 ? 0 : idx + 1; };
};

}  // namespace dectnrp::mac
