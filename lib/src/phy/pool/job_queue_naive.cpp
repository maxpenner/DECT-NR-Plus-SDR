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

#include "dectnrp/phy/pool/job_queue_naive.hpp"

#ifdef PHY_POOL_JOB_QUEUE_NAIVE_CONDITION_VARIABLE_OR_BUSY_WAITING
#else
#include "dectnrp/common/thread/watch.hpp"
#endif

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

job_queue_naive_t::job_queue_naive_t(const uint32_t id_, const uint32_t capacity_)
    : job_queue_base_t(id_, capacity_),
      enqueue_ptr(0),
      dequeue_ptr(0) {
    for (uint32_t job_slot_id = 0; job_slot_id < capacity; ++job_slot_id) {
        job_slot_vec.push_back(job_slot_t(job_slot_id));
    }

    // queue is empty at the beginning
    job_slot_vec_cnt.store(0, std::memory_order_release);
}

bool job_queue_naive_t::enqueue_nto(job_t&& job) {
    if (!permeable.load(std::memory_order_acquire)) {
        return true;
    }

    bool ret = false;

    lockv.lock();
    ret = enqueue_under_lock(std::move(job));
    lockv.unlock();

#ifdef PHY_POOL_JOB_QUEUE_NAIVE_CONDITION_VARIABLE_OR_BUSY_WAITING
    if (ret) {
        cv.notify_all();
    }
#endif

    return ret;
}

std::vector<std::string> job_queue_naive_t::report_start() const {
    std::vector<std::string> lines;

    std::string str("Job Queue " + std::to_string(id));
    str.append(" #Jobs " + std::to_string(job_slot_vec.size()));

    lines.push_back(str);

    return lines;
}

std::vector<std::string> job_queue_naive_t::report_stop() const {
    std::vector<std::string> lines;

    // indicate how many jobs are unprocessed
    std::string str_residual("Job Queue " + std::to_string(id) + " Slots Filled " +
                             std::to_string(get_used()));
    lines.push_back(str_residual);

    for (auto& elem : job_slot_vec) {
        std::string str("Job Queue " + std::to_string(id));

        str.append(" Job " + std::to_string(elem.id));
        str.append(" Filled " + std::to_string(elem.stats.filled));
        str.append(" Processed " + std::to_string(elem.stats.processed));

        lines.push_back(str);
    }

    return lines;
}

bool job_queue_naive_t::wait_for_new_job_to(job_t& job) {
    bool ret = false;

#ifdef PHY_POOL_JOB_QUEUE_NAIVE_CONDITION_VARIABLE_OR_BUSY_WAITING
    // upon entering function, get the lock on the mutex
    std::unique_lock<std::mutex> lk(lockv);

    /* Leave this function only when either ...
     *
     * 1) There is a job, i.e. ret = true.
     * 2) The condition variable timed out.
     */
    while (ret == false) {
        // enter next while loop if no jobs are available
        while (job_slot_vec_cnt.load(std::memory_order_acquire) == 0) {
            // no jobs available, so implicitly unlock mutex and wait
            auto res = cv.wait_for(lk, std::chrono::milliseconds(JOB_QUEUE_WAIT_TIMEOUT_MS));

            // at this point, this thread has been woken up (either notified, timeout or spurious)
            // and we have the lock on the mutex.

            // cv timed out, leave function without job and by that remove the lock on the mutex
            if (res == std::cv_status::timeout) {
                return false;
            }
        }

        // we have the lock on the mutex, check if there is a job available
        ret = dequeue_under_lock(job);
    }
#else
    common::watch_t watch;

    /* Leave this function only when either ...
     *
     * 1) There is a job, i.e. ret = true.
     * 2) The watch timed out.
     */
    while (ret == false) {
        // any jobs available?
        if (job_slot_vec_cnt.load(std::memory_order_acquire) > 0) {
            // lock and try to get the job
            lockv.lock();
            ret = dequeue_under_lock(job);
            lockv.unlock();
        } else {
            // limit calls to atomic
            common::watch_t::busywait_us();
        }

        // first check for timeout
        if (watch.is_elapsed<common::milli>(JOB_QUEUE_WAIT_TIMEOUT_MS)) {
            break;
        }
    }

#endif

    return ret;
}

bool job_queue_naive_t::enqueue_under_lock(job_t&& job) {
    // is there a free slot?
    if (get_free() == 0) {
#ifdef PHY_POOL_JOB_QUEUE_JOB_SLOT_UNAVAILABILITY_FATAL_OR_DISCARD
        dectnrp_assert_failure("no free job slot");
#endif
        return false;
    }

    job.fifo_cnt = fifo_cnt++;

    job_slot_vec[enqueue_ptr].job = job;

    job_slot_vec_cnt.fetch_add(1, std::memory_order_release);

    enqueue_ptr = (enqueue_ptr + 1) % capacity;

    ++job_slot_vec[enqueue_ptr].stats.filled;

    return true;
}

bool job_queue_naive_t::dequeue_under_lock(job_t& job) {
    // queue can be empty
    if (get_used() == 0) {
        return false;
    }

    job = job_slot_vec[dequeue_ptr].job;

    job_slot_vec_cnt.fetch_add(-1, std::memory_order_release);

    dequeue_ptr = (dequeue_ptr + 1) % capacity;

    ++job_slot_vec[dequeue_ptr].stats.processed;

    return true;
}

uint32_t job_queue_naive_t::get_free() const {
    if (dequeue_ptr > enqueue_ptr) {
        // w_idx should never reach r_idx
        return dequeue_ptr - enqueue_ptr - 1;
    }

    // w_idx should never reach r_idx
    return dequeue_ptr + capacity - enqueue_ptr - 1;
}

uint32_t job_queue_naive_t::get_used() const {
    if (enqueue_ptr >= dequeue_ptr) {
        return enqueue_ptr - dequeue_ptr;
    }

    return enqueue_ptr + capacity - dequeue_ptr;
}

}  // namespace dectnrp::phy
