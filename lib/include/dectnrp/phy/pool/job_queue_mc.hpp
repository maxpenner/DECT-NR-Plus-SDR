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

#include <cstdint>
#include <memory>

#include "dectnrp/phy/pool/job_queue_base.hpp"
#include "header_only/cameron314/blockingconcurrentqueue.h"

#define PHY_POOL_JOB_QUEUE_MC_MUTEX_OR_SPINLOCK
#ifdef PHY_POOL_JOB_QUEUE_MC_MUTEX_OR_SPINLOCK
#include <mutex>
#else
#include "dectnrp/common/thread/spinlock.hpp"
#endif

namespace dectnrp::phy {

class job_queue_mc_t final : public job_queue_base_t {
    public:
        /**
         * By default, moodycamel::ConcurrentQueue is not a FIFO queue when it comes to dequeuing
         * jobs. That is, if there are multiple producers there is no absolute order for the jobs
         * but only for each producer's sub-queue,
         * https://github.com/cameron314/concurrentqueue/issues/52.
         *
         * For instance, we could have five jobs with IDs 102, 103, 104, 105 and 106 in the queue.
         * Assuming two producers and two consumers, the consumers could dequeue 104 and 105 (both
         * from first producer) before 102, 103 and 104 (from second producer), which leads to a
         * fatal program error because 104 and 105 must be fully processed once dequeued (i.e. they
         * block the consumers), but they can only be started once 102, 103 and 104 are finished.
         * Thus, no further jobs are dequeued and the job queue quickly overflows.
         *
         * As a countermeasure, moodycamel::ConcurrentQueue allows using tokens. We can use a single
         * producer token for all producers, which enforces a single producer queue and thus an
         * absolute order when dequeuing. A downside is that access to the token must be
         * thread-safe, for which we use a mutex or spinlock.
         * https://github.com/cameron314/concurrentqueue?tab=readme-ov-file#advanced-features.
         */
        explicit job_queue_mc_t(const uint32_t id_, const uint32_t capacity_);
        ~job_queue_mc_t() = default;

        job_queue_mc_t() = delete;
        job_queue_mc_t(const job_queue_mc_t&) = delete;
        job_queue_mc_t& operator=(const job_queue_mc_t&) = delete;
        job_queue_mc_t(job_queue_mc_t&&) = delete;
        job_queue_mc_t& operator=(job_queue_mc_t&&) = delete;

        bool enqueue_nto(job_t&& job) override final;

        friend class worker_pool_t;
        friend class worker_tx_rx_t;

    private:
        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

        bool wait_for_new_job_to(job_t& job) override final;

        moodycamel::BlockingConcurrentQueue<job_t> job_vec;
        std::unique_ptr<moodycamel::ProducerToken> ptok;

#ifdef PHY_POOL_JOB_QUEUE_MC_MUTEX_OR_SPINLOCK
        std::mutex lockv;
#else
        common::spinlock_t lockv;
#endif
};

}  // namespace dectnrp::phy
