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
#include "dectnrp/mac/ppx/pll_pps_param.hpp"

namespace dectnrp::mac::ppx {

pll_t::pll_t(const section3::duration_t beacon_period_)
    : beacon_period(beacon_period_),
      new_value_64(beacon_period.get_samp_rate() / 1000 * PPX_PPS_PLL_PARAM_NEW_VALUE_MS),
      separation_min_64(beacon_period.get_samp_rate() / 1000 * PPX_PPS_PLL_PARAM_MIN_SEPARATION_MS),
      separation_max_64(beacon_period.get_samp_rate() / 1000 *
                        PPX_PPS_PLL_PARAM_MAX_SEPARATION_MS) {
    dectnrp_assert(beacon_period.get_samp_rate() % 1000 == 0, "ill-defined");
    dectnrp_assert(new_value_64 < separation_min_64, "ill-defined");
    dectnrp_assert(separation_min_64 < separation_max_64, "ill-defined");

    dectnrp_assert(separation_min_64 + beacon_period.get_N_samples_64() * 5 <= separation_max_64,
                   "ill-defined");
    dectnrp_assert(separation_min_64 % new_value_64 == 0, "ill-defined");

    beacon_time_vec.resize(separation_min_64 / new_value_64, common::adt::UNDEFINED_EARLY_64);
};

void pll_t::provide_measured_beacon_time(const int64_t beacon_time_64) {
    const int64_t A = beacon_time_64 - beacon_time_vec.at(prev_idx());

    if (A < new_value_64) {
        return;
    }

    beacon_time_vec.at(idx) = beacon_time_64;

    if (beacon_time_vec.at(next_idx()) < 0) {
        idx = next_idx();
        return;
    }

    const int64_t separation_64 = beacon_time_vec.at(idx) - beacon_time_vec.at(next_idx());

    idx = next_idx();

    const int64_t B = common::adt::round_integer(separation_64, beacon_period.get_N_samples_64());

    warp_factor =
        warp_factor * ema_alpha + (1.0 - ema_alpha) * static_cast<float>(separation_64) /
                                      static_cast<float>(B * beacon_period.get_N_samples_64());
}

int64_t pll_t::get_warped(const int64_t length) const {
    const float A = std::round(static_cast<float>(length) * warp_factor);

    return static_cast<int64_t>(A);
}

}  // namespace dectnrp::mac::ppx
