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

#include <array>
#include <cstdint>
#include <utility>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part3/derivative/duration.hpp"

namespace dectnrp::section3 {

class duration_lut_t {
    public:
        duration_lut_t() = default;
        explicit duration_lut_t(const uint32_t samp_rate_,
                                const uint32_t turn_around_time_us = 0,
                                const uint32_t settling_time_freq_us = 0,
                                const uint32_t settling_time_gain_us = 0);

        static int64_t get_N_samples_from_duration(const uint32_t samp_rate,
                                                   const duration_ec_t duration_ec,
                                                   const uint32_t mult = 1);

        static int64_t get_N_samples_from_us(const uint32_t samp_rate, const uint32_t us);

        // ##################################################
        // create an instance of duration_t (internal values depend on samp_rate)

        duration_t get_duration(const duration_ec_t duration_ec, const uint32_t mult = 1) const {
            return duration_t(duration_ec, mult, get_N_samples_from_duration(duration_ec, mult));
        }

        // ##################################################
        // number of samples in a duration

        int64_t get_N_samples_from_duration(const duration_ec_t duration_ec,
                                            const uint32_t mult = 1) const {
            return duration_vec.at(std::to_underlying(duration_ec)) * static_cast<int64_t>(mult);
        }

        duration_ec_t get_duration_ec_subslots(const uint32_t u) const;

        int64_t get_N_samples_from_subslots(const uint32_t u, const uint32_t mult = 1) const;

        // ##################################################
        // number of samples at a point in time

        int64_t get_N_samples_at_last_full_second(const int64_t current_sample_count_64) const;
        int64_t get_N_samples_at_next_full_second(const int64_t current_sample_count_64) const;

        // ##################################################
        // duration of a number of samples

        /**
         * \brief A int64_t can count nanoseconds for 2^63/(1e9*60*60*24*365)=292.47 years. This is
         * more than enough for unix time, and since our hardware sample rate will always be below
         * 1GS/s, it is also enough for DECT times.
         *
         * \param N_samples_64
         * \return
         */
        int64_t get_N_ns_from_samples(const int64_t N_samples_64) const;

        int64_t get_N_us_from_samples(const int64_t N_samples_64) const {
            return get_N_ns_from_samples(N_samples_64) / int64_t{1000};
        }

    private:
        uint32_t samp_rate;

        std::array<int64_t, std::to_underlying(duration_ec_t::CARDINALITY)> duration_vec;
};

}  // namespace dectnrp::section3
