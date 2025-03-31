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

#include "dectnrp/sections_part3/derivative/duration_ec.hpp"

namespace dectnrp::section3 {

class duration_t {
    public:
        duration_t() = default;

        /**
         * \brief Creating an instance of duration_t is only possible if the hardware sample rate is
         * known, otherwise N_samples cannot be determined. The sample rate is determined at
         * runtime. Hence, durations should be requested from duration_lut_t.
         *
         * \param duration_ec_
         * \param mult_
         * \param N_samples
         */
        explicit duration_t(const duration_ec_t duration_ec_,
                            const uint32_t mult_,
                            const uint32_t N_samples_)
            : duration_ec(duration_ec_),
              mult(mult_),
              N_samples(N_samples_),
              N_samples_64(static_cast<int64_t>(N_samples_)) {}

        friend bool operator>(duration_t& lhs, duration_t& rhs) {
            return lhs.N_samples_64 > rhs.N_samples_64 ? true : false;
        }

        friend bool operator<(duration_t& lhs, duration_t& rhs) {
            return lhs.N_samples_64 < rhs.N_samples_64 ? true : false;
        }

        duration_ec_t duration_ec{duration_ec_t::CARDINALITY};
        uint32_t mult{1};
        uint32_t N_samples{0};
        int64_t N_samples_64{0};
};

}  // namespace dectnrp::section3
