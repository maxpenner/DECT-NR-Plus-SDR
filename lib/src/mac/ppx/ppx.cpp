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

#include "dectnrp/mac/ppx/ppx.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::mac {

ppx_t::ppx_t(const section3::duration_t ppx_period_,
             const section3::duration_t ppx_length_,
             const section3::duration_t ppx_time_advance_,
             const section3::duration_t beacon_period_,
             const section3::duration_t time_deviation_max_)
    : ppx_period(ppx_period_),
      ppx_length(ppx_length_),
      ppx_time_advance(ppx_time_advance_),
      beacon_period(beacon_period_),
      time_deviation_max(time_deviation_max_),
      ppx_period_warped_64(ppx_period.get_N_samples<int64_t>()) {
    dectnrp_assert(ppx_length < ppx_period, "ill-defined");
    dectnrp_assert(ppx_time_advance < ppx_period, "ill-defined");
    dectnrp_assert(beacon_period < ppx_period, "ill-defined");
    dectnrp_assert(
        ppx_period.get_N_samples<int64_t>() % beacon_period.get_N_samples<int64_t>() == 0,
        "ill-defined");
};

void ppx_t::set_ppx_rising_edge(const int64_t ppx_rising_edge_64) {
    dectnrp_assert(ppx_rising_edge_estimation_64 < 0, "already initialized");
    dectnrp_assert(0 < ppx_rising_edge_64, "rising edge time must be positive");

    ppx_rising_edge_estimation_64 = ppx_rising_edge_64;
}

void ppx_t::extrapolate_next_rising_edge() {
    ppx_rising_edge_estimation_64 += ppx_period_warped_64;
}

void ppx_t::provide_beacon_time(const int64_t beacon_time_64) {
    provide_beacon_time_out_of_raster(beacon_time_64, beacon_period.get_N_samples<int64_t>());
}

void ppx_t::provide_beacon_time_out_of_raster(const int64_t beacon_time_64,
                                              const int64_t beacon_period_custom_64) {
    dectnrp_assert(0 <= ppx_rising_edge_estimation_64, "not initialized yet");

    // determine deviation between received beacon time and extrapolated beacon time
    const auto deviation =
        determine_offset(ppx_rising_edge_estimation_64, beacon_period_custom_64, beacon_time_64);

    dectnrp_assert(std::abs(deviation) <= time_deviation_max.get_N_samples<int64_t>(),
                   "synchronization lost");

    // slight adjustment
    ppx_rising_edge_estimation_64 += deviation;
}

radio::pulse_config_t ppx_t::get_ppx_imminent() const {
    const int64_t A = ppx_rising_edge_estimation_64 + ppx_period_warped_64;

    return radio::pulse_config_t(A, A + ppx_length.get_N_samples<int64_t>());
}

int64_t ppx_t::determine_offset(const int64_t ref_64,
                                const int64_t raster_64,
                                const int64_t time_to_test_64) {
    dectnrp_assert(raster_64 % 2 == 0, "raster must be even");

    const int64_t A = time_to_test_64 - ref_64;
    const int64_t B = common::adt::round_integer(A, raster_64);
    const int64_t C = ref_64 + B * raster_64;

    dectnrp_assert(std::abs(time_to_test_64 - C) < raster_64 / 2, "distance incorrect");

    return time_to_test_64 - C;
}

}  // namespace dectnrp::mac
