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
#include <memory>

#define PHY_POOL_TOKEN_CONDITION_VARIABLE_OR_BUSY_WAITING
#ifdef PHY_POOL_TOKEN_CONDITION_VARIABLE_OR_BUSY_WAITING
#include <condition_variable>
#include <mutex>
#else
#include <atomic>

#include "dectnrp/common/thread/spinlock.hpp"
#endif

#include "dectnrp/limits.hpp"

namespace dectnrp::phy {

class token_t {
    public:
        token_t(const uint32_t n_pools_);

        token_t() = delete;
        token_t(const token_t&) = delete;
        token_t& operator=(const token_t&) = delete;
        token_t(token_t&&) = delete;
        token_t& operator=(token_t&&) = delete;

        [[nodiscard]] static std::shared_ptr<token_t> create(const uint32_t n_pools) {
            return std::make_shared<token_t>(n_pools);
        }

        /**
         * \brief Locks only if internal fifo_cnt is equal to fifo_cnt_. Has a timeout (to).
         *
         * \param fifo_cnt_
         * \return lock acquired
         * \return timed out
         */
        bool lock_fifo_to(const uint32_t id_caller, const int64_t fifo_cnt_);
        void unlock_fifo();

        /// locks as soon as possible
        void lock(const uint32_t id_caller);
        bool try_lock(const uint32_t id_caller);
        void unlock();

        /// retrieve ID of holder, call only when holding the lock
        uint32_t get_id_holder() const;

    private:
        static constexpr uint32_t TOKEN_WAIT_TIMEOUT_MS = 100;

#ifdef PHY_POOL_TOKEN_CONDITION_VARIABLE_OR_BUSY_WAITING
        std::mutex lockv;
        std::condition_variable cv;
        std::array<int64_t, limits::max_nof_radio_phy_pairs_one_tpoint> fifo_cnt;
#else
        common::spinlock_t lockv;
        std::array<std::atomic<int64_t>, limits::max_nof_radio_phy_pairs_one_tpoint> fifo_cnt;
#endif

        /// initial value is an invalid ID
        uint32_t id_holder = limits::max_nof_radio_phy_pairs_one_tpoint;
};

}  // namespace dectnrp::phy
