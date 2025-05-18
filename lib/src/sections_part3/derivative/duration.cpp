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

#include "dectnrp/sections_part3/derivative/duration.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

namespace dectnrp::section3 {

duration_t::duration_t(const uint32_t samp_rate_,
                       const duration_ec_t duration_ec_,
                       const uint32_t mult_)
    : samp_rate(samp_rate_),
      duration_ec(duration_ec_),
      mult(mult_) {
    dectnrp_assert(0 < samp_rate, "zero sample rate");
    dectnrp_assert(duration_ec != duration_ec_t::CARDINALITY, "cardinality");
    dectnrp_assert(0 < mult, "zero multiplier");

    uint32_t samp_rate_divisor = 0;

    switch (duration_ec) {
        using enum duration_ec_t;
        case ms001:
            samp_rate_divisor = 1000;
            break;
        case s001:
            samp_rate_divisor = 1;
            break;
        case slot001:
            samp_rate_divisor = constants::slots_per_sec;
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
        case CARDINALITY:
            dectnrp_assert_failure("cardinality");
            break;
    }

    dectnrp_assert(samp_rate > 0, "must be positive");
    dectnrp_assert(samp_rate_divisor > 0, "must be positive");
    dectnrp_assert(samp_rate % samp_rate_divisor == 0, "must have zero remainder");

    N_samples = samp_rate / samp_rate_divisor;

    dectnrp_assert(N_samples > 0, "cannot be zero");

    // make sure multiplies is not too large, we divide by two for some leeway
    dectnrp_assert(static_cast<int64_t>(N_samples) * static_cast<int64_t>(mult) <=
                       std::numeric_limits<uint32_t>::max() / 2,
                   "out-of-bound");

    N_samples *= mult;
}

}  // namespace dectnrp::section3
