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

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"

namespace dectnrp::common::adt {

template <typename R, typename... Args>
class callbacks_t {
    public:
        callbacks_t() = default;

        /// signature every callback must adhere to, however, callbacks may not use all arguments
        // https://stackoverflow.com/questions/29609866/stdbind-makes-no-sense-to-me-whatsoever
        typedef std::function<R(
            const int64_t now_64, const size_t idx, int64_t& next_64, int64_t& period_64, Args...)>
            cb_t;

        std::optional<size_t> add_callback(cb_t callback,
                                           const int64_t next_64,
                                           const int64_t period_64) {
            dectnrp_assert(!is_in_callback, "changing callback from callback");

            auto it = std::find_if(callbacks.begin(), callbacks.end(), [](const auto& elem) {
                return elem.cb == nullptr;
            });

            if (it == callbacks.end()) {
                return std::nullopt;
            }

            it->cb = callback;
            it->next_64 = next_64;
            it->period_64 = period_64;

            set_it_next();

            return std::distance(std::begin(callbacks), it);
        }

        /// https://stackoverflow.com/questions/20833453/comparing-stdfunctions-for-equality
        void rm_callback(const size_t idx) {
            dectnrp_assert(!is_in_callback, "changing callback from callback");
            callbacks.at(idx).cb = callback_entry_t();
            set_it_next();
        }

        void update_next(const size_t idx, const size_t next_64) {
            dectnrp_assert(!is_in_callback, "changing callback from callback");
            callbacks.at(idx).next_64 = next_64;
            set_it_next();
        }

        void adjust_next(const size_t idx, const size_t next_adjustment_64) {
            dectnrp_assert(!is_in_callback, "changing callback from callback");
            callbacks.at(idx).next_64 += next_adjustment_64;
            set_it_next();
        }

        void update_period(const size_t idx, const size_t period_64) {
            dectnrp_assert(!is_in_callback, "changing callback from callback");
            callbacks.at(idx).period_64 = period_64;
        }

        void adjust_period(const size_t idx, const size_t period_adjustment_64) {
            dectnrp_assert(!is_in_callback, "changing callback from callback");
            callbacks.at(idx).period_64 += period_adjustment_64;
        }

        R run(const int64_t now_64, Args... args)
            requires(std::is_same_v<R, void>)
        {
            is_in_callback = true;

            while (it_next->next_64 <= now_64) {
                dectnrp_assert(now_64 < it_next->next_64 + it_next->period_64, "callback skipped");

                it_next->cb(now_64,
                            std::distance(std::begin(callbacks), it_next),
                            it_next->next_64,
                            it_next->period_64,
                            args...);

                dectnrp_assert(it_next->next_64 > 0, "next must be positive");
                dectnrp_assert(it_next->period_64 > 0, "period must be positive");

                it_next->next_64 += it_next->period_64;

                dectnrp_assert(now_64 < it_next->next_64, "adjusted period before now_64");

                set_it_next();
            }

            is_in_callback = false;
        }

        [[nodiscard]] std::optional<R> run(const int64_t now_64, Args... args)
            requires(!std::is_same_v<R, void>)
        {
            is_in_callback = true;

            std::optional<R> ret{std::nullopt};

            while (it_next->next_64 <= now_64) {
                dectnrp_assert(now_64 < it_next->next_64 + it_next->period_64, "callback skipped");

                ret = it_next->cb(now_64,
                                  std::distance(std::begin(callbacks), it_next),
                                  it_next->next_64,
                                  it_next->period_64,
                                  args...);

                dectnrp_assert(it_next->next_64 > 0, "next must be positive");
                dectnrp_assert(it_next->period_64 > 0, "period must be positive");

                it_next->next_64 += it_next->period_64;

                dectnrp_assert(now_64 < it_next->next_64, "adjusted period before now_64");

                set_it_next();
            }

            is_in_callback = false;

            return ret;
        }

    private:
        struct callback_entry_t {
                cb_t cb{nullptr};
                int64_t next_64{std::numeric_limits<int64_t>::max()};
                int64_t period_64{0};
        };

        std::array<callback_entry_t, limits::max_callbacks> callbacks;

        /// index of next callback, so we don't have to search the array with every call of run()
        decltype(callbacks.begin()) it_next{nullptr};

        // update time and index of next callback to call
        void set_it_next() {
            it_next =
                std::min_element(callbacks.begin(), callbacks.end(), [](auto& lhs, auto& rhs) {
                    return lhs.next_64 < rhs.next_64;
                });
        }

        /**
         * \brief Removing and updating callbacks from callbacks is not possible as this would
         * change the value of it_next. This guard variable prevents this mistake.
         */
        bool is_in_callback{false};
};

}  // namespace dectnrp::common::adt
