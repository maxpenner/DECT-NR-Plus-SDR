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

#include "dectnrp/phy/pool/job_queue_mc.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/pool/job_queue_base.hpp"

namespace dectnrp::phy {

job_queue_mc_t::job_queue_mc_t(const uint32_t id_, const uint32_t capacity_)
    : job_queue_base_t(id_, capacity_) {
    dectnrp_assert(capacity >= 32, "moodycamel capacity must be at least 32");

    // https://github.com/cameron314/concurrentqueue/blob/master/blockingconcurrentqueue.h#L58
    job_vec = moodycamel::BlockingConcurrentQueue<job_t>(capacity * 6);

    ptok = std::make_unique<moodycamel::ProducerToken>(job_vec);
}

bool job_queue_mc_t::enqueue_nto(job_t&& job) {
    if (!permeable.load(std::memory_order_acquire)) {
        return true;
    }

    // producer token must be thread-safe
    lockv.lock();

    job.fifo_cnt = fifo_cnt;

    if (job_vec.try_enqueue(*ptok, job)) {
        // increment only if job made it into the queue
        ++fifo_cnt;

        lockv.unlock();

        return true;
    }

#ifdef PHY_POOL_JOB_QUEUE_JOB_SLOT_UNAVAILABILITY_FATAL_OR_DISCARD
    dectnrp_assert_failure("no free job slot");
#endif

    lockv.unlock();

    return false;
}

std::vector<std::string> job_queue_mc_t::report_start() const {
    std::vector<std::string> lines;

    std::string str("Job Queue " + std::to_string(id));
    str.append(" #Jobs " + std::to_string(capacity));

    lines.push_back(str);

    return lines;
}

std::vector<std::string> job_queue_mc_t::report_stop() const {
    std::vector<std::string> lines;

    // indicate how many jobs are unprocessed
    std::string str_residual("Job Queue " + std::to_string(id));
    lines.push_back(str_residual);

    return lines;
}

bool job_queue_mc_t::wait_for_new_job_to(job_t& job) {
    return job_vec.wait_dequeue_timed(job, std::chrono::milliseconds(JOB_QUEUE_WAIT_TIMEOUT_MS));
}

}  // namespace dectnrp::phy
