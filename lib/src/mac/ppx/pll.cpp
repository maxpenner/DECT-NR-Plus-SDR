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

#include "dectnrp/mac/ppx/pll.hpp"

#include <cmath>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/mac/ppx/pll_param.hpp"
#include "dectnrp/sections_part2/reference_time.hpp"

namespace dectnrp::mac::ppx {

pll_t::pll_t(const section3::duration_t beacon_period_)
    : beacon_period(beacon_period_),
      new_value_64(beacon_period.get_samp_rate<int64_t>() * PPX_PLL_PARAM_NEW_VALUE_MS / 1000),
      separation_min_64(beacon_period.get_samp_rate<int64_t>() * PPX_PLL_PARAM_SEPARATION_MIN_MS /
                        1000),
      separation_max_64(beacon_period.get_samp_rate<int64_t>() * PPX_PLL_PARAM_SEPARATION_MAX_MS /
                        1000) {
    dectnrp_assert(beacon_period.get_samp_rate<int64_t>() <= new_value_64, "ill-defined");
    dectnrp_assert(new_value_64 < separation_min_64, "ill-defined");
    dectnrp_assert(separation_min_64 < separation_max_64, "ill-defined");
    dectnrp_assert(new_value_64 * 2 <= separation_max_64 - separation_min_64, "ill-defined");
    dectnrp_assert(separation_min_64 % new_value_64 == 0, "ill-defined");
    dectnrp_assert(separation_max_64 * section2::get_reference_time_accuracy(true).accuracy_ppm *
                           2 / int64_t{1000000} <
                       beacon_period.get_samp_rate<int64_t>() / 4,
                   "ill-defined");

    beacon_time_vec.resize(separation_min_64 / new_value_64, common::adt::UNDEFINED_EARLY_64);
};

void pll_t::provide_beacon_time(const int64_t beacon_time_64) {
    const int64_t A = beacon_time_64 - beacon_time_vec.at(prev_idx());

    // is the separation between this beacon and the past beacon large enough?
    if (A < new_value_64) {
        return;
    }

    beacon_time_vec.at(idx) = beacon_time_64;

    // oldest value in array must be positive, otherwise there are not enough beacon times yet
    if (beacon_time_vec.at(next_idx()) < 0) {
        idx = next_idx();
        return;
    }

    const int64_t separation_64 = beacon_time_vec.at(idx) - beacon_time_vec.at(next_idx());

    idx = next_idx();

    /* This estimation will always be correct as long as the beacons are provided in their correct
     * raster, and as long as the clock deviation in ppm is not too large. This is checked in the
     * constructor.
     */
    const int64_t most_likely_number_of_beacon_periods_in_separation =
        common::adt::round_integer(separation_64, beacon_period.get_N_samples<int64_t>());

    const int64_t equivalent_separation_in_rx_time_base =
        most_likely_number_of_beacon_periods_in_separation * beacon_period.get_N_samples<int64_t>();

    const float warp_factor_update = static_cast<float>(separation_64) /
                                     static_cast<float>(equivalent_separation_in_rx_time_base);

    warp_factor = warp_factor * PPX_PLL_PARAM_EMA_ALPHA +
                  (1.0 - PPX_PLL_PARAM_EMA_ALPHA) * warp_factor_update;
}

}  // namespace dectnrp::mac::ppx
