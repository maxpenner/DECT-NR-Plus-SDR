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
#include <atomic>
#include <cstdint>

#define PHY_POOL_IRREGULAR_MUTEX_OR_SPINLOCK
#ifdef PHY_POOL_IRREGULAR_MUTEX_OR_SPINLOCK
#include <mutex>
#else
#include "dectnrp/common/thread/spinlock.hpp"
#endif

#include "dectnrp/limits.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"

namespace dectnrp::phy {

class irregular_queue_t {
    public:
        irregular_queue_t();
        ~irregular_queue_t() = default;

        irregular_queue_t(const irregular_queue_t&) = delete;
        irregular_queue_t& operator=(const irregular_queue_t&) = delete;
        irregular_queue_t(irregular_queue_t&&) = delete;
        irregular_queue_t& operator=(irregular_queue_t&&) = delete;

        void push(const irregular_report_t&& irregular_report);

        [[nodiscard]] irregular_report_t pop();

        [[nodiscard]] int64_t get_next_time() const;

    private:
        std::atomic<int64_t> next_time_64{irregular_report_t::undefined_late};

        std::array<irregular_report_t, limits::max_irregular_callback_pending> irregular_report_arr;

        decltype(irregular_report_arr.begin()) it_next{nullptr};

#ifdef PHY_POOL_IRREGULAR_MUTEX_OR_SPINLOCK
        mutable std::mutex lockv;
#else
        mutable common::spinlock_t lockv;
#endif
};

}  // namespace dectnrp::phy
