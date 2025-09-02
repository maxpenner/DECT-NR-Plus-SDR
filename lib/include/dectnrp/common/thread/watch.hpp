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

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "dectnrp/common/prog/compiler.hpp"

namespace dectnrp::common {

using nano = std::chrono::nanoseconds;
using micro = std::chrono::microseconds;
using milli = std::chrono::milliseconds;
using seconds = std::chrono::seconds;

using steady_clock = std::chrono::steady_clock;
using system_clock = std::chrono::system_clock;

#ifdef GCC_VERSION
#if GCC_VERSION >= 130000
using utc_clock = std::chrono::utc_clock;
using tai_clock = std::chrono::tai_clock;
using gps_clock = std::chrono::gps_clock;
#else
using utc_clock = system_clock;
using tai_clock = system_clock;
using gps_clock = system_clock;
#endif
#endif

#ifdef CLANG_VERSION
#if CLANG_VERSION >= 180000
using utc_clock = std::chrono::utc_clock;
using tai_clock = std::chrono::tai_clock;
using gps_clock = std::chrono::gps_clock;
#else
using utc_clock = system_clock;
using tai_clock = system_clock;
using gps_clock = system_clock;
#endif
#endif

class watch_t {
    public:
        watch_t();

        /// reset internal time reference
        void reset();

        /**
         * \brief Elapsed since construction or call of reset().
         *
         * \tparam Ret return type
         * \tparam Res resolution type
         */
        template <std::integral Ret = int64_t, typename Res = nano>
            requires(std::is_same_v<Ret, int64_t> || std::is_same_v<Ret, uint64_t>)
        Ret get_elapsed() const {
            const auto elapsed = std::chrono::steady_clock::now() - start;
            return static_cast<Ret>(std::chrono::duration_cast<Res>(elapsed).count());
        }

        /*   12*365*86400 (12 years from 1958 to 1970)
         * +      3*86400 (leap years 1960, 1964, 1968)
         * +           10 (initial offset)
         * =    378691210
         */
        static constexpr std::chrono::duration epoch_tai_utc_sec{std::chrono::seconds(378691210)};

        /*   10*365*86400 (10 years from 1970 to 1980)
         * +      5*86400 (5 day offset to 6th of January)
         * +      2*86400 (leap years 1972, 1976)
         * +            9 (leap seconds)
         * =    315964809
         */
        static constexpr std::chrono::duration epoch_gps_utc_sec{std::chrono::seconds(-315964809)};

        template <std::integral Ret = int64_t, typename Res = nano, typename Clock = utc_clock>
            requires(std::is_same_v<Ret, int64_t> || std::is_same_v<Ret, uint64_t>)
        static Ret get_elapsed_since_epoch() {
            const auto elapsed = Clock::now().time_since_epoch();
            return static_cast<Ret>(std::chrono::duration_cast<Res>(elapsed).count());
        }

        template <typename Res = nano>
        bool is_elapsed(const int64_t target) const {
            return get_elapsed<int64_t, Res>() >= target;
        }

        template <typename Res = micro>
        static void sleep(const int64_t target) {
            if constexpr (std::is_same_v<Res, micro>) {
                sleep_us(target);
            } else if constexpr (std::is_same_v<Res, milli>) {
                sleep_us(target * int64_t{1000});
            } else {
                static_assert(std::is_same_v<Res, seconds>, "undefined sleep duration");
                sleep_us(target * int64_t{1000000});
            }
        }

#ifdef COMMON_THREAD_WATCH_ENABLE_BUSYWAIT
        template <typename Res = micro>
        static void busywait(const int64_t target) {
            if constexpr (std::is_same_v<Res, micro>) {
                busywait_us(target);
            } else if constexpr (std::is_same_v<Res, milli>) {
                busywait_us(target * int64_t{1000});
            } else {
                static_assert(std::is_same_v<Res, seconds>, "undefined sleep duration");
                busywait_us(target * int64_t{1000000});
            }
        }
#endif

        /**
         * \brief Sleep until the specified target time. Target time is given as time elapsed since
         * epoch.
         *
         * \param target
         * \tparam Res
         * \tparam Clock
         * \return true if sleep called
         * \return false if sleep not called
         */
        template <typename Res = micro, typename Clock = utc_clock>
        static bool sleep_until(const int64_t target) {
            const int64_t elapsed = get_elapsed_since_epoch<int64_t, Res, Clock>();

            if (target <= elapsed) {
                return false;
            }

            sleep<Res>(target - elapsed);

            return true;
        }

#ifdef COMMON_THREAD_WATCH_ENABLE_BUSYWAIT
        template <typename Res = micro, typename Clock = utc_clock>
        static bool busywait_until(const int64_t target) {
            const int64_t elapsed = get_elapsed_since_epoch<int64_t, Res, Clock>();

            if (target <= elapsed) {
                return false;
            }

            busywait<Res>(target - elapsed);

            return true;
        }
#endif

        /**
         * \brief A busywait as an alternative to sleep. The internals of this busywait function are
         * hideous and hence only considered to be an experimental feature to test ultra low latency
         * use cases. Wherever a busywait is used in this project, there is always a stable, classic
         * solution with the usual synchronization primitives (mutex, condition variable etc.) right
         * next to it. Same goes for spinlocks.
         *
         * In general, busywaits/spinlocks are relatively hard to implement correctly, especially
         * when mixed with manual settings of thread priority, scheduling algorithm, core isolation,
         * number of threads compared to cores etc.
         *
         * https://www.realworldtech.com/forum/?threadid=189711&curpostid=189723
         * https://www.realworldtech.com/forum/?threadid=189711&curpostid=189752
         *
         * \param microseconds_
         */
        static void busywait_us(const uint32_t microseconds_ = 5);

        static std::string get_date_and_time();

    private:
        /// stopwatch variables
        std::chrono::time_point<std::chrono::steady_clock> start;

        /// sleep functionality using POSIX's nanosleep internally
        static void sleep_us(const int64_t microseconds_);
};

}  // namespace dectnrp::common
