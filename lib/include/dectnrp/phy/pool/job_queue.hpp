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

/**
 * Whenever any producer has a job to put into the queue, it must do so by only trying without
 * blocking. Making a producer thread wait for the queue to get a free slot is not a viable option
 * due to the additional latency and code complexity. However, if we only do a single-shot try, we
 * either have a free slot or we don't. If there is no free slot, we have to decide whether this is
 * a fatal error, or whether we simply discard the job. The default case should be a fatal error as
 * randomly discarded jobs are hard to debug. Thus, the job queue should be large enough to
 * temporarily buffer a large burst of incoming jobs, but ultimately consumers must consume jobs
 * faster than producers produce them.
 *
 * NOTE: The fatal error is triggered by an assert, so asserts must be enabled.
 */
#define PHY_POOL_JOB_QUEUE_JOB_SLOT_UNAVAILABILITY_FATAL_OR_DISCARD

/**
 * We can either use a custom naive queue or moodycamel::ConcurrentQueue, which is complex MPMC
 * queue. By default, moodycamel::ConcurrentQueue is not a FIFO queue when it comes to dequeuing
 * jobs. That is, if there are multiple producers there is no absolute order for the jobs but only
 * for each producer's sub-queue, https://github.com/cameron314/concurrentqueue/issues/52.
 *
 * For instance, we could have five jobs with IDs 102, 103, 104, 105 and 106 in the queue. Assuming
 * two producers and two consumers, the consumers could dequeue 104 and 105 (both from first
 * producer) before 102, 103 and 104 (from second producer), which leads to a fatal program error
 * because 104 and 105 must be fully processed once dequeued (i.e. they block the consumers), but
 * they can only be started once 102, 103 and 104 are finished. Thus, no further jobs are dequeued
 * and the job queue quickly overflows.
 *
 * As a countermeasure, moodycamel::ConcurrentQueue allows using tokens. We can use a single
 * producer token for all producers, which enforces a single producer queue and thus an absolute
 * order when dequeuing. A downside is that access to the token must be thread-safe, for which we
 * use a spinlock.
 * https://github.com/cameron314/concurrentqueue?tab=readme-ov-file#advanced-features.
 */
// #define PHY_POOL_JOB_QUEUE_NAIVE_OR_MOODYCAMEL

// ######################################################################### NAIVE
// ######################################################################### NAIVE
// ######################################################################### NAIVE
#ifdef PHY_POOL_JOB_QUEUE_NAIVE_OR_MOODYCAMEL

#include <atomic>
#include <cstdint>
#include <vector>

#include "dectnrp/common/reporting.hpp"
#include "dectnrp/phy/pool/job.hpp"
#include "dectnrp/phy/pool/job_slot.hpp"

/* How does the job queue notify job consumers about the availability of new jobs? When this
 * directive is defined, the function wait_for_new_job_to() internally waits on a conditional
 * variables, which is notified from enqueue_under_lock(). The wait itself has a timeout. If this
 * directive is commented out, the function wait_for_new_job() busy waits with the same timeout.
 */
#define PHY_POOL_JOB_QUEUE_NAIVE_USES_CONDITION_VARIABLE_OR_BUSYWAITING
#ifdef PHY_POOL_JOB_QUEUE_NAIVE_USES_CONDITION_VARIABLE_OR_BUSYWAITING
#include <condition_variable>
#include <mutex>
#else
#include "dectnrp/common/thread/spinlock.hpp"
#endif

namespace dectnrp::phy {

class job_queue_t final : public common::reporting_t {
    public:
        explicit job_queue_t(const uint32_t id_, const uint32_t capacity_);
        ~job_queue_t() = default;

        job_queue_t() = delete;
        job_queue_t(const job_queue_t&) = delete;
        job_queue_t& operator=(const job_queue_t&) = delete;
        job_queue_t(job_queue_t&&) = delete;
        job_queue_t& operator=(job_queue_t&&) = delete;

        static constexpr uint32_t JOB_QUEUE_WAIT_TIMEOUT_MS = 100;

        /**
         * \brief producers (for instance worker_sync_t), function has no timeout (nto)
         *
         * \param job
         * \return true when was was enqueued
         * \return false if not
         */
        bool enqueue_nto(job_t&& job);

        void set_permeable() { permeable.store(true, std::memory_order_release); }
        void set_impermeable() { permeable.store(false, std::memory_order_release); }

        const uint32_t id;
        const uint32_t capacity;

        friend class worker_pool_t;   // access to private report functions
        friend class worker_tx_rx_t;  // access to wait_for_new_job_to()

    private:
        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

        std::atomic<bool> permeable{false};

        // assigned in job_queue_t
        int64_t fifo_cnt;

        /// pointer pointing to entries in job_slot_vec, jobs are enqueued FIFO
        uint32_t enqueue_ptr;
        uint32_t dequeue_ptr;

        std::vector<job_slot_t> job_slot_vec;
        /// consumers keep trying to dequeue jobs until this global counter is zero
        std::atomic<int32_t> job_slot_vec_cnt;

        /// internal function called from enqueue_nto()
        bool enqueue_under_lock(job_t&& job);

        /// consumers (for instance worker_tx_rx_t), function has a timeout (to)
        bool wait_for_new_job_to(job_t& job);

        /// internal function called from wait_for_new_job_to()
        bool dequeue_under_lock(job_t& job);

        /// convenience function to get current job counts
        uint32_t get_free() const;
        uint32_t get_used() const;

#ifdef PHY_POOL_JOB_QUEUE_NAIVE_USES_CONDITION_VARIABLE_OR_BUSYWAITING
        std::mutex lockv;
        std::condition_variable cv;
#else
        common::spinlock_t lockv;
#endif
};

}  // namespace dectnrp::phy

// ######################################################################### NAIVE
// ######################################################################### NAIVE
// ######################################################################### NAIVE
#else
// ######################################################################### MOODYCAMEL
// ######################################################################### MOODYCAMEL
// ######################################################################### MOODYCAMEL

#include <atomic>
#include <cstdint>
#include <memory>

#include "dectnrp/common/reporting.hpp"
#include "dectnrp/external/cameron314/blockingconcurrentqueue.h"
#include "dectnrp/phy/pool/job.hpp"
#include "dectnrp/phy/pool/job_slot.hpp"

// #define PHY_POOL_JOB_QUEUE_MOODYCAMEL_USES_MUTEX_OR_SPINLOCK
#ifdef PHY_POOL_JOB_QUEUE_MOODYCAMEL_USES_MUTEX_OR_SPINLOCK
#include <mutex>
#else
#include "dectnrp/common/thread/spinlock.hpp"
#endif

namespace dectnrp::phy {

class job_queue_t final : public common::reporting_t {
    public:
        explicit job_queue_t(const uint32_t id_, const uint32_t capacity_);
        ~job_queue_t() = default;

        job_queue_t() = delete;
        job_queue_t(const job_queue_t&) = delete;
        job_queue_t& operator=(const job_queue_t&) = delete;
        job_queue_t(job_queue_t&&) = delete;
        job_queue_t& operator=(job_queue_t&&) = delete;

        static constexpr uint32_t JOB_QUEUE_WAIT_TIMEOUT_MS = 100;

        /**
         * \brief producers (for instance worker_sync_t), function has no timeout (nto)
         *
         * \param job
         * \return true when was was enqueued
         * \return false if not
         */
        bool enqueue_nto(job_t&& job);

        void set_permeable() { permeable.store(true, std::memory_order_release); }
        void set_impermeable() { permeable.store(false, std::memory_order_release); }

        const uint32_t id;
        const uint32_t capacity;

        friend class worker_pool_t;   // access to private report functions
        friend class worker_tx_rx_t;  // access to wait_for_new_job_to()

    private:
        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

        std::atomic<bool> permeable{false};

        // assigned in job_queue_t
        int64_t fifo_cnt{0};

        moodycamel::BlockingConcurrentQueue<job_t> job_vec;
        std::unique_ptr<moodycamel::ProducerToken> ptok;

        /// consumers (for instance worker_tx_rx_t), function has a timeout (to)
        bool wait_for_new_job_to(job_t& job);

#ifdef PHY_POOL_JOB_QUEUE_MOODYCAMEL_USES_MUTEX_OR_SPINLOCK
        std::mutex lockv;
#else
        common::spinlock_t lockv;
#endif
};

}  // namespace dectnrp::phy

#endif
// ######################################################################### MOODYCAMEL
// ######################################################################### MOODYCAMEL
// ######################################################################### MOODYCAMEL
