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

#include "dectnrp/apps/stream.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::apps {

stream_t::stream_t(const std::size_t id_,
                   const double period_sec_,
                   const double offset_sec_,
                   const std::size_t repetitions_)
    : id(id_),
      period_sec(period_sec_),
      offset_sec(offset_sec_),
      repetitions(repetitions_) {
    dectnrp_assert((0.999 * 1e-3) <= period_sec, "period too short {}", period_sec);
    dectnrp_assert(0.0 <= offset_sec, "offset too small {}", offset_sec);
    dectnrp_assert(offset_sec <= (1.0 - period_sec), "offset too large {}", offset_sec);
    dectnrp_assert(1 <= repetitions, "one repetition required");
    dectnrp_assert(offset_sec + period_sec * static_cast<double>(repetitions - 1) < 1.0,
                   "last repetition too late");
    dectnrp_assert(repetitions_cnt == 0, "counter not at zero");
}

int64_t stream_t::set_start_full_sec(const int64_t full_sec_64) {
    full_sec_cnt_64 = full_sec_64;
    return get_next_us();
}

int64_t stream_t::get_next_us() {
    const double offset = offset_sec + static_cast<double>(repetitions_cnt) * period_sec;

    const int64_t ret = full_sec_cnt_64 * mega +
                        static_cast<int64_t>(std::round(offset * static_cast<double>(mega)));

    dectnrp_assert(ret < ((full_sec_cnt_64 + 1) * mega), "too late");

    ++repetitions_cnt;

    // all repetitions done for this second?
    if (repetitions_cnt == repetitions) {
        repetitions_cnt = 0;
        ++full_sec_cnt_64;
    }

    return ret;
}

void stream_t::generate_payload([[maybe_unused]] uint8_t* payload,
                                [[maybe_unused]] const std::size_t payload_size_byte) {
    // ToDo

    ++stats.tx;
}

}  // namespace dectnrp::apps
