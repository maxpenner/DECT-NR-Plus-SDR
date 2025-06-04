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

#include <atomic>
#include <cstdint>
#include <vector>

#include "dectnrp/phy/pool/job.hpp"
#include "dectnrp/phy/pool/job_queue_base.hpp"

#define PHY_POOL_JOB_QUEUE_NAIVE_CONDITION_VARIABLE_OR_BUSY_WAITING
#ifdef PHY_POOL_JOB_QUEUE_NAIVE_CONDITION_VARIABLE_OR_BUSY_WAITING
#include <condition_variable>
#include <mutex>
#else
#include "dectnrp/common/thread/spinlock.hpp"
#endif

namespace dectnrp::phy {

class job_queue_naive_t final : public job_queue_base_t {
    public:
        explicit job_queue_naive_t(const uint32_t id_, const uint32_t capacity_);
        ~job_queue_naive_t() = default;

        job_queue_naive_t() = delete;
        job_queue_naive_t(const job_queue_naive_t&) = delete;
        job_queue_naive_t& operator=(const job_queue_naive_t&) = delete;
        job_queue_naive_t(job_queue_naive_t&&) = delete;
        job_queue_naive_t& operator=(job_queue_naive_t&&) = delete;

        bool enqueue_nto(job_t&& job) override final;

        friend class worker_pool_t;
        friend class worker_tx_rx_t;

    private:
        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

        bool wait_for_new_job_to(job_t& job) override final;

        /// pointer pointing to entries in job_slot_vec, jobs are enqueued FIFO
        uint32_t enqueue_ptr;
        uint32_t dequeue_ptr;

        struct job_slot_t {
                explicit job_slot_t(const uint32_t id_)
                    : id(id_) {};

                const uint32_t id;
                job_t job;

                /// statistics of the slot
                struct stats_t {
                        int64_t filled{0};
                        int64_t processed{0};
                };

                stats_t stats;
        };

        std::vector<job_slot_t> job_slot_vec;

        /// consumers keep trying to dequeue jobs until this global counter is zero
        std::atomic<int32_t> job_slot_vec_cnt;

        /// internal function called from enqueue_nto()
        bool enqueue_under_lock(job_t&& job);

        /// internal function called from wait_for_new_job_to()
        bool dequeue_under_lock(job_t& job);

        /// convenience function to get current job counts
        uint32_t get_free() const;
        uint32_t get_used() const;

#ifdef PHY_POOL_JOB_QUEUE_NAIVE_CONDITION_VARIABLE_OR_BUSY_WAITING
        std::mutex lockv;
        std::condition_variable cv;
#else
        common::spinlock_t lockv;
#endif
};

}  // namespace dectnrp::phy
