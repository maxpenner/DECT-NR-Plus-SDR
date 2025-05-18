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

#include "dectnrp/application/app_client.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::application {

app_client_t::app_client_t(const uint32_t id_,
                           const common::threads_core_prio_config_t thread_config_,
                           phy::job_queue_t& job_queue_,
                           const uint32_t N_queue,
                           const queue_size_t queue_size)
    : app_t(id_, thread_config_, job_queue_, N_queue, queue_size),
      indicator_cnt{0} {}

void app_client_t::trigger_forward_nto(const uint32_t datagram_cnt) {
#ifdef APP_CLIENT_USES_CONDITION_VARIABLE_OR_BUSYWAITING
    lockv.lock();
#else
#endif

    inc_indicator_cnt_under_lock(datagram_cnt);

#ifdef APP_CLIENT_USES_CONDITION_VARIABLE_OR_BUSYWAITING
    lockv.unlock();
    cv.notify_all();
#endif
}

void app_client_t::work_sc() {
    // external exit condition
    while (keep_running.load(std::memory_order_acquire)) {
#ifdef APP_CLIENT_USES_CONDITION_VARIABLE_OR_BUSYWAITING

        // get the lock on the mutex
        std::unique_lock<std::mutex> lk(lockv);

        do {
            // we hold the lock, so forward new data if there is any
            if (indicator_cnt > 0) {
                forward_under_lock();
            }

            // no more, or not enough, new data available, so implicitly unlock and wait
            auto res = cv.wait_for(lk, std::chrono::milliseconds(APP_POLL_WAIT_TIMEOUT_MS));

            // cv timed out, leave while loop and check external exit condition
            if (res == std::cv_status::timeout) {
                break;
            }

        } while (1);
#else
        common::watch_t watch;

        do {
            // check if there is new data available, if so forward it
            if (indicator_cnt.load(std::memory_order_acquire) > 0) {
                forward_under_lock();
                watch.reset();
            } else {
                // limit calls to atomic
                common::watch_t::busywait_us();
            }

            // watch timed out, leave while loop and check external exit condition
            if (watch.is_elapsed<common::milli>(APP_POLL_WAIT_TIMEOUT_MS)) {
                break;
            }

        } while (1);
#endif
    }
};

void app_client_t::inc_indicator_cnt_under_lock(const uint32_t datagram_cnt) {
    // ToDo: use conn_idx for better efficiency

    indicator_cnt += datagram_cnt;
}

void app_client_t::dec_indicator_cnt_under_lock() {
    // ToDo: use conn_idx for better efficiency

    indicator_cnt--;
}

uint32_t app_client_t::get_indicator_cnt_under_lock() {
    // ToDo: return conn_idx for better efficiency

    return indicator_cnt;
}

void app_client_t::forward_under_lock() {
    // process until all indicated datagrams have been processed
    while (get_indicator_cnt_under_lock() > 0) {
#ifdef ENABLE_ASSERT
        bool all_queues_empty = true;
#endif

        // check for every existing interface
        for (uint32_t i = 0; i < get_n_connections(); ++i) {
            // check if there is any data
            const uint32_t n = copy_from_queue_to_localbuffer(i);

            // if so ...
            if (n > 0) {
                if (!filter_egress_datagram(i)) {
                    continue;
                }

                // ... forward
                write_immediate(i, buffer_local, n);
                dec_indicator_cnt_under_lock();

#ifdef ENABLE_ASSERT
                all_queues_empty = false;
#endif
            }
        }

#ifdef ENABLE_ASSERT
        dectnrp_assert(!all_queues_empty, "went over all queues and non has data");
#endif
    }
}

}  // namespace dectnrp::application
