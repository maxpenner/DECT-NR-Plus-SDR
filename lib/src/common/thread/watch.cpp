/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/common/thread/watch.hpp"

#include "header_only/hinnant/date.h"

namespace dectnrp::common {

watch_t::watch_t() { reset(); }

void watch_t::reset() { start = std::chrono::steady_clock::now(); }

static uint32_t burn_cycles_locally() {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < 999; ++i) {
        sum += i;
    }
    return sum;
}

void watch_t::busywait_us(const uint32_t microseconds_) {
    //__builtin_ia32_pause();

    const auto start = std::chrono::steady_clock::now();

    const int64_t elapsed_ns_target = static_cast<int64_t>(microseconds_) * 1000;
    int64_t elapsed_ns = 0;

    do {
        const auto elapsed = std::chrono::steady_clock::now() - start;
        elapsed_ns = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());

        /**
         * \brief To reduce calls of std::chrono::steady_clock::now(), which may be called by
         * multiple busywaiting threads, burn some cycles locally.
         */
        burn_cycles_locally();

    } while (elapsed_ns < elapsed_ns_target);
}

std::string watch_t::get_date_and_time() {
    return date::format("%F %T UTC", std::chrono::system_clock::now());
}

void watch_t::sleep_us(const int64_t microseconds_) {
    int result = 0;

    struct timespec ts_remaining = {microseconds_ / 1000000, (microseconds_ % 1000000) * 1000};

    do {
        struct timespec ts_sleep = ts_remaining;
        result = nanosleep(&ts_sleep, &ts_remaining);
    } while ((EINTR == errno) && (-1 == result));
}

}  // namespace dectnrp::common
