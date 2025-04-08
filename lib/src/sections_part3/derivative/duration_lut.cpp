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
#include <utility>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

namespace dectnrp::section3 {

duration_lut_t::duration_lut_t(const uint32_t samp_rate_,
                               const uint32_t turn_around_time_us,
                               const uint32_t settling_time_freq_us,
                               const uint32_t settling_time_gain_us)
    : samp_rate(static_cast<int64_t>(samp_rate_)) {
    // these values also probably don't divide samp_rate with zero remainder
    duration_vec.at(std::to_underlying(duration_ec_t::us100)) =
        get_N_samples_from_us(samp_rate, 100);
    duration_vec.at(std::to_underlying(duration_ec_t::turn_around_time_us)) =
        get_N_samples_from_us(samp_rate, turn_around_time_us);
    duration_vec.at(std::to_underlying(duration_ec_t::settling_time_freq_us)) =
        get_N_samples_from_us(samp_rate, settling_time_freq_us);
    duration_vec.at(std::to_underlying(duration_ec_t::settling_time_gain_us)) =
        get_N_samples_from_us(samp_rate, settling_time_gain_us);

    // all of these values should divide samp_rate with zero remainder
    using dt = std::underlying_type_t<duration_ec_t>;
    for (dt i = static_cast<dt>(duration_ec_t::ms001);
         i <= static_cast<dt>(duration_ec_t::subslot_u8_001);
         ++i) {
        duration_vec.at(i) = get_N_samples_from_duration(samp_rate, static_cast<duration_ec_t>(i));
    }
}

int64_t duration_lut_t::get_N_samples_from_duration(const uint32_t samp_rate,
                                                    const duration_ec_t duration_ec,
                                                    const uint32_t mult) {
    int64_t samp_rate_divisor = 0;

    switch (duration_ec) {
        using enum duration_ec_t;
        case us100:
            samp_rate_divisor = 10000;
            break;
        case ms001:
            samp_rate_divisor = 1000;
            break;
        case s001:
            samp_rate_divisor = 1;
            break;
        case slot001:
            samp_rate_divisor = 2400;
            break;
        case subslot_u1_001:
            samp_rate_divisor = constants::u1_subslots_per_sec;
            break;
        case subslot_u2_001:
            samp_rate_divisor = constants::u2_subslots_per_sec;
            break;
        case subslot_u4_001:
            samp_rate_divisor = constants::u4_subslots_per_sec;
            break;
        case subslot_u8_001:
            samp_rate_divisor = constants::u8_subslots_per_sec;
            break;
        case turn_around_time_us:
        case settling_time_freq_us:
        case settling_time_gain_us:
        case CARDINALITY:
            dectnrp_assert_failure("cannot be called with static function");
            break;
    }

    dectnrp_assert(samp_rate > 0 && samp_rate_divisor > 0, "cannot be zero");
    dectnrp_assert(samp_rate % samp_rate_divisor == 0, "cannot have non-zero remainder");

    const int64_t ret = static_cast<int64_t>(samp_rate) / samp_rate_divisor;

    dectnrp_assert(ret > 0, "cannot be zero");

    return ret * static_cast<int64_t>(mult);
}

int64_t duration_lut_t::get_N_samples_from_us(const uint32_t samp_rate, const uint32_t us) {
    dectnrp_assert(us <= 1000000, "should not be larger than one second");
    return static_cast<int64_t>(samp_rate) * static_cast<int64_t>(us) / int64_t{1000000};
}

duration_ec_t duration_lut_t::get_duration_ec_subslots(const uint32_t u) const {
    dectnrp_assert(std::has_single_bit(u) && u <= 8, "u undefined");

    switch (u) {
        case 1:
            return duration_ec_t::subslot_u1_001;
        case 2:
            return duration_ec_t::subslot_u2_001;
        case 4:
            return duration_ec_t::subslot_u4_001;
    }

    return duration_ec_t::subslot_u8_001;
}

int64_t duration_lut_t::get_N_samples_from_subslots(const uint32_t u, const uint32_t mult) const {
    return get_N_samples_from_duration(get_duration_ec_subslots(u), mult);
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
    dectnrp_assert(samp_rate % duration.N_samples == 0, "second not a multiple of duration");

    return samp_rate / duration.N_samples;
}

}  // namespace dectnrp::section3
