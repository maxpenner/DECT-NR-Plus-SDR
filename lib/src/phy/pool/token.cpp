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

#include "dectnrp/phy/pool/token.hpp"

#include <algorithm>
#include <limits>

#ifdef PHY_POOL_TOKEN_CONDITION_VARIABLE_OR_BUSY_WAITING
#else
#include "dectnrp/common/thread/watch.hpp"
#endif

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

token_t::token_t(const uint32_t n_pools_) {
    dectnrp_assert(n_pools_ <= limits::max_nof_radio_phy_pairs_one_tpoint, "too many pools");

    // set all fifo counts to very large value, this way we trigger an assert if we ever wait for
    // any job as it will be definitely earlier
    std::fill(fifo_cnt.begin(), fifo_cnt.end(), std::numeric_limits<int64_t>::max());

    // set used values to start value that will also be used by job_queue
    std::fill_n(fifo_cnt.begin(), n_pools_, 0);
};

bool token_t::lock_fifo_to(const uint32_t id_caller, const int64_t fifo_cnt_) {
#ifdef PHY_POOL_TOKEN_CONDITION_VARIABLE_OR_BUSY_WAITING
    {
        std::unique_lock<std::mutex> lk(lockv);

        dectnrp_assert(fifo_cnt[id_caller] <= fifo_cnt_, "waiting for earlier job");

        while (fifo_cnt[id_caller] != fifo_cnt_) {
            // implicitly unlock mutex and wait
            const auto res = cv.wait_for(lk, std::chrono::milliseconds(TOKEN_WAIT_TIMEOUT_MS));

            // cv timed out, leave function and by that remove the lock on the mutex
            if (res == std::cv_status::timeout) {
                return false;
            }
        }
    }
#else
    common::watch_t watch;

    dectnrp_assert(fifo_cnt[id_caller].load(std::memory_order_acquire) <= fifo_cnt_,
                   "waiting for earlier job");

    while (fifo_cnt[id_caller].load(std::memory_order_acquire) != fifo_cnt_) {
        // limit calls to atomic
        common::watch_t::busywait_us();

        // timeout after some time to allow checking external exit condition
        if (watch.is_elapsed<common::milli>(TOKEN_WAIT_TIMEOUT_MS)) {
            return false;
        }
    }
#endif

    // at this point fifo_cnt has the correct value and we can lock the token
    lock(id_caller);

    return true;
}

void token_t::unlock_fifo() {
    dectnrp_assert(id_holder < limits::max_nof_radio_phy_pairs_one_tpoint, "holder ID invalid");

#ifdef PHY_POOL_TOKEN_CONDITION_VARIABLE_OR_BUSY_WAITING
    ++fifo_cnt[id_holder];
#else
    fifo_cnt[id_holder].fetch_add(1);
#endif

    unlock();

#ifdef PHY_POOL_TOKEN_CONDITION_VARIABLE_OR_BUSY_WAITING
    cv.notify_all();
#else
#endif
}

void token_t::lock(const uint32_t id_caller) {
    lockv.lock();

    dectnrp_assert(id_holder >= limits::max_nof_radio_phy_pairs_one_tpoint,
                   "holder ID already valid");

    id_holder = id_caller;
}

bool token_t::try_lock(const uint32_t id_caller) {
    if (lockv.try_lock()) {
        dectnrp_assert(id_holder >= limits::max_nof_radio_phy_pairs_one_tpoint,
                       "holder ID already valid");

        id_holder = id_caller;

        return true;
    }

    return false;
}

void token_t::unlock() {
    // set to invalid ID
    id_holder = limits::max_nof_radio_phy_pairs_one_tpoint;

    lockv.unlock();
}

uint32_t token_t::get_id_holder() const {
    dectnrp_assert(id_holder < limits::max_nof_radio_phy_pairs_one_tpoint, "holder ID invalid");

    return id_holder;
}

}  // namespace dectnrp::phy
