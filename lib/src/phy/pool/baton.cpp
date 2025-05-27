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

#include "dectnrp/phy/pool/baton.hpp"

#include <limits>
#include <utility>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"

#ifdef PHY_POOL_BATON_USES_CONDITION_VARIABLE_OR_BUSYWAITING
#else
#include "dectnrp/common/thread/watch.hpp"
#endif

namespace dectnrp::phy {

baton_t::baton_t(const uint32_t nof_worker_sync_,
                 const int64_t sync_time_unique_limit_64_,
                 const uint32_t rx_job_regular_period_)
    :
#ifdef ENABLE_ASSERT
      chunk_time_end_64(std::numeric_limits<int64_t>::max()),
#endif
      nof_worker_sync(nof_worker_sync_),
      sync_time_unique_limit_64(sync_time_unique_limit_64_),
      rx_job_regular_period(rx_job_regular_period_),
      register_cnt(0),
      register_now_64(common::adt::UNDEFINED_EARLY_64),
      id_holder(0),
      /* This time must initially be negative enough so that the first packet's synchronization
       * time, which will be 0 or positive, is definitely detected as unique.
       */
      sync_time_last_64(common::adt::UNDEFINED_EARLY_64),
      rx_job_regular_period_cnt(0) {
}

void baton_t::set_tpoint_to_notify(upper::tpoint_t* tpoint_, std::shared_ptr<token_t> token_) {
    tpoint = tpoint_;
    token = token_;
};

std::pair<int64_t, phy::irregular_report_t> baton_t::register_and_wait_for_others_nto(
    const int64_t now_64) {
    std::unique_lock<std::mutex> lk(register_mtx);

    ++register_cnt;

    // later time?
    if (register_now_64 < now_64) {
        register_now_64 = now_64;
    }

    irregular_report_t irregular_report;

    // last one?
    if (register_cnt == nof_worker_sync) {
        // lock with id_caller=0, always valid
        if (!token->try_lock(0)) {
            dectnrp_assert_failure("unable to lock token");
        }
        irregular_report = tpoint->work_start_imminent(register_now_64);
        token->unlock();

        register_cv.notify_all();
    }

    dectnrp_assert(register_cnt <= nof_worker_sync, "too many registrations");

    // wait for other instances
    while (register_cnt != nof_worker_sync) {
        register_cv.wait(lk);
    }

    dectnrp_assert(register_cnt == nof_worker_sync, "not all registrations");

    return std::make_pair(register_now_64, irregular_report);
}

bool baton_t::is_id_holder_the_same(const uint32_t id_caller) const {
#ifdef PHY_POOL_BATON_USES_CONDITION_VARIABLE_OR_BUSYWAITING
    std::unique_lock<std::mutex> lk(mtx);
    return id_caller == id_holder;
#else
    return id_caller == id_holder.load(std::memory_order_acquire);
#endif
}

bool baton_t::wait_to(const uint32_t id_target) {
#ifdef PHY_POOL_BATON_USES_CONDITION_VARIABLE_OR_BUSYWAITING
    std::unique_lock<std::mutex> lk(mtx);

    while (id_holder != id_target) {
        // implicitly unlock mutex and wait
        const auto res = cv.wait_for(lk, std::chrono::milliseconds(BATON_WAIT_TIMEOUT_MS));

        // cv timed out, leave function and by that remove the lock on the mutex
        if (res == std::cv_status::timeout) {
            return false;
        }
    }

#else
    common::watch_t watch;

    while (id_holder.load(std::memory_order_acquire) != id_target) {
        // limit calls to atomic
        common::watch_t::busywait_us();

        // timeout after some time to allow checking external exit condition
        if (watch.is_elapsed<common::milli>(BATON_WAIT_TIMEOUT_MS)) {
            return false;
        }
    }
#endif

    return true;
}

void baton_t::pass_on(const uint32_t id_caller) {
    dectnrp_assert(is_id_holder_the_same(id_caller),
                   "caller does not hold baton, yet checking sync time");

#ifdef PHY_POOL_BATON_USES_CONDITION_VARIABLE_OR_BUSYWAITING
    {
        std::unique_lock<std::mutex> lk(mtx);

        id_holder = (id_holder + 1) % nof_worker_sync;
    }

    cv.notify_all();
#else
    id_holder.store((id_holder.load(std::memory_order_acquire) + 1) % nof_worker_sync,
                    std::memory_order_release);
#endif
}

bool baton_t::is_sync_time_unique(const int64_t sync_time_candidate_64) {
    // get time difference to last sync time
    const int64_t sync_time_diff_64 = sync_time_candidate_64 - sync_time_last_64;

    // is this a unique packet, i.e. not a double detection?
    if (sync_time_diff_64 > sync_time_unique_limit_64) {
        // store new latest unique sync time
        sync_time_last_64 = sync_time_candidate_64;
        return true;
    }

    return false;
}

bool baton_t::is_job_regular_due() {
    if (rx_job_regular_period == 0) {
        return false;
    }

    ++rx_job_regular_period_cnt;

    if (rx_job_regular_period_cnt == rx_job_regular_period) {
        rx_job_regular_period_cnt = 0;
        return true;
    }

    return false;
}

}  // namespace dectnrp::phy
