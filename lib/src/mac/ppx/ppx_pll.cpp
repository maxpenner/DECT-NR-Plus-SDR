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

#include "dectnrp/mac/ppx/ppx_pll.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::mac::ppx {

ppx_pll_t::ppx_pll_t(section3::duration_t ppx_period_,
                     const section3::duration_t ppx_length_,
                     const section3::duration_t ppx_time_advance_,
                     const section3::duration_t raster_,
                     const section3::duration_t time_deviation_max_)
    : ppx_period(ppx_period_),
      ppx_length(ppx_length_),
      ppx_time_advance(ppx_time_advance_),
      raster(raster_),
      time_deviation_max(time_deviation_max_) {
    dectnrp_assert(ppx_length < ppx_period, "ppx_period must be longest");
    dectnrp_assert(ppx_time_advance < ppx_period, "ppx_period must be longest");
    dectnrp_assert(raster < ppx_period, "ppx_period must be longest");
};

void ppx_pll_t::set_ppx_time(const int64_t ppx_time_64) {
    dectnrp_assert(!((0 <= ppx_time_estimation_64) &&
                     std::abs(determine_offset(
                         ppx_time_estimation_64, ppx_period.N_samples_64, ppx_time_64)) <=
                         time_deviation_max.N_samples_64),
                   "synchronization lost");

    // we consider the beacon time to be our new reference
    ppx_time_estimation_64 = ppx_time_64;
}

void ppx_pll_t::set_ppx_time_extrapolation(const int64_t now_64) {
    dectnrp_assert(ppx_time_estimation_64 < now_64, "too early");

    ppx_time_estimation_64 += ppx_period.N_samples_64;

    dectnrp_assert(now_64 < ppx_time_estimation_64, "too late");
}

void ppx_pll_t::set_ppx_time_in_raster(const int64_t time_in_raster_64) {
    set_ppx_time_in_raster_custom(time_in_raster_64, raster.N_samples_64);
}

void ppx_pll_t::set_ppx_time_in_raster_custom(const int64_t time_in_raster_custom_64,
                                              const int64_t raster_64) {
    dectnrp_assert(0 <= ppx_time_estimation_64, "first beacon must be time align beacon");

    // determine deviation between received beacon time and extrapolated beacon time
    const auto deviation =
        determine_offset(ppx_time_estimation_64, raster_64, time_in_raster_custom_64);

    dectnrp_assert(std::abs(deviation) <= time_deviation_max.N_samples_64, "synchronization lost");

    // slightly adjust the master beacon time
    ppx_time_estimation_64 += deviation;
}

radio::pulse_config_t ppx_pll_t::get_ppx_imminent(const int64_t now_64) {
    dectnrp_assert(ppx_time_estimation_64 < now_64, "too early");

    const int64_t A = ppx_time_estimation_64 + ppx_period.N_samples_64;

    dectnrp_assert(now_64 < A, "too late");

    return radio::pulse_config_t(A, A + ppx_length.N_samples_64);
}

int64_t ppx_pll_t::determine_offset(const int64_t ref_64,
                                    const int64_t raster_64,
                                    const int64_t now_64) {
    const int64_t A = now_64 - ref_64;
    const int64_t B = common::adt::round_integer(A, raster_64);
    const int64_t C = ref_64 + B * raster_64;
    return now_64 - C;
}

}  // namespace dectnrp::mac::ppx
