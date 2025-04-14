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

#include "dectnrp/mac/pll/pll.hpp"

#include <cmath>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/mac/pll/pll_param.hpp"
#include "dectnrp/sections_part2/reference_time.hpp"

namespace dectnrp::mac {

pll_t::pll_t(const section3::duration_t beacon_period_)
    : beacon_period(beacon_period_),
      dist_min_accept_64(beacon_period.get_samp_rate<int64_t>() * PLL_PARAM_DIST_MIN_ACCEPT_MS /
                         1000),
      dist_min_64(beacon_period.get_samp_rate<int64_t>() * PLL_PARAM_DIST_MIN_MS / 1000),
      dist_max_64(dist_min_64 + beacon_period.get_N_samples<int64_t>() *
                                    PLL_PARAM_DIST_MIN_TO_MAX_IN_BEACON_PERIODS) {
    dectnrp_assert(dist_min_accept_64 < dist_min_64, "ill-defined");
    dectnrp_assert(dist_min_64 % dist_min_accept_64 == 0, "ill-defined");
    dectnrp_assert(dist_min_64 < dist_max_64, "ill-defined");
    dectnrp_assert(
        dist_max_64 * section2::get_reference_time_accuracy(true).accuracy_ppm / int64_t{1000000} <
            beacon_period.get_N_samples<int64_t>() / PLL_PARAM_DIST_MAX_DEVIATION_SAFETY_FACTOR,
        "ill-defined");

    beacon_time_vec.resize(dist_min_64 / dist_min_accept_64, common::adt::UNDEFINED_EARLY_64);

    warp_factor_ema = common::adt::ema_t<double, double>(1.0f, PLL_PARAM_EMA_ALPHA);
};

void pll_t::provide_beacon_time(const int64_t beacon_time_64) {
    const int64_t A = beacon_time_64 - get_beacon_time_last_known();

    // is the dist between this beacon and the past beacon large enough?
    if (A < dist_min_accept_64) {
        return;
    }

    beacon_time_vec.at(idx) = beacon_time_64;

    // oldest value must be positive, otherwise there are not enough beacon times collected yet
    if (beacon_time_vec.at(next_idx()) < 0) {
        idx = next_idx();
        return;
    }

    // time distance to known oldest value
    const int64_t dist_64 = beacon_time_vec.at(idx) - get_beacon_time_oldest_known();

    // ignore if dist is too large
    if (dist_max_64 < dist_64) {
        idx = next_idx();
        return;
    }

    idx = next_idx();

    /* This estimation will always be correct as long as the beacons are provided in their correct
     * raster, and as long as the clock deviation in ppm is not too large. This is checked in the
     * constructor.
     */
    const int64_t most_likely_number_of_beacon_periods_in_dist =
        common::adt::round_integer(dist_64, beacon_period.get_N_samples<int64_t>());

    const int64_t equivalent_dist_in_rx_time_base =
        most_likely_number_of_beacon_periods_in_dist * beacon_period.get_N_samples<int64_t>();

    const double warp_factor_latest =
        static_cast<double>(dist_64) / static_cast<double>(equivalent_dist_in_rx_time_base);

#ifdef PLL_PARAM_UNLUCKY_WARP_FACTOR_FILTER
    if (warp_factor_latest < 1.0f + PLL_PARAM_PPM_OUT_OF_SYNC / 1.0e6) {
        return;
    }

    if (1.0f - PLL_PARAM_PPM_OUT_OF_SYNC / 1.0e6 < warp_factor_latest) {
        return;
    }
#endif

    warp_factor_ema.update(warp_factor_latest);

    dectnrp_assert(warp_factor_ema.get_val() < 1.0 + PLL_PARAM_PPM_OUT_OF_SYNC / 1.0e6,
                   "warp_factor too large");
    dectnrp_assert(1.0 - PLL_PARAM_PPM_OUT_OF_SYNC / 1.0e6 < warp_factor_ema.get_val(),
                   "warp_factor too small");
}

void pll_t::reset() {
    std::fill(beacon_time_vec.begin(), beacon_time_vec.end(), common::adt::UNDEFINED_EARLY_64);
    idx = 0;
    warp_factor_ema.set_val(1.0);
}

double pll_t::convert_warp_factor_to_ppm() const {
    return (warp_factor_ema.get_val() - 1.0) * 1.0e6;
};

}  // namespace dectnrp::mac
