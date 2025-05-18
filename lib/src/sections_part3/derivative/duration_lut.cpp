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

#include "dectnrp/sections_part3/derivative/duration_lut.hpp"

#include <cmath>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section3 {

duration_lut_t::duration_lut_t(const uint32_t samp_rate_)
    : samp_rate(static_cast<int64_t>(samp_rate_)) {
    using dt = std::underlying_type_t<duration_ec_t>;
    for (dt i = static_cast<dt>(duration_ec_t::ms001);
         i < static_cast<dt>(duration_ec_t::CARDINALITY);
         ++i) {
        duration_vec.at(i) =
            duration_t(samp_rate, static_cast<duration_ec_t>(i)).get_N_samples<int64_t>();
    }
}

int64_t duration_lut_t::get_N_samples_from_subslots(const uint32_t u, const uint32_t mult) const {
    return get_N_samples_from_duration(get_duration_ec_depending_on_mu(u), mult);
}

int64_t duration_lut_t::get_N_samples_at_last_full_second(
    const int64_t current_sample_count_64) const {
    return common::adt::multiple_leq(current_sample_count_64, static_cast<int64_t>(samp_rate));
}

int64_t duration_lut_t::get_N_samples_at_next_full_second(
    const int64_t current_sample_count_64) const {
    return common::adt::multiple_geq(current_sample_count_64, static_cast<int64_t>(samp_rate));
}

int64_t duration_lut_t::get_N_ns_from_samples(const int64_t N_samples_64) const {
    // determine the number of full seconds
    const int64_t A = N_samples_64 / static_cast<int64_t>(samp_rate);

    // then determine the number of fractional samples
    const int64_t B = N_samples_64 % static_cast<int64_t>(samp_rate);

    return A * int64_t{1000000000} + B * int64_t{1000000000} / static_cast<int64_t>(samp_rate);
}

uint32_t duration_lut_t::get_N_duration_in_second(const duration_t duration) const {
    dectnrp_assert(samp_rate % duration.get_N_samples() == 0, "second not a multiple of duration");

    return samp_rate / duration.get_N_samples();
}

}  // namespace dectnrp::section3
