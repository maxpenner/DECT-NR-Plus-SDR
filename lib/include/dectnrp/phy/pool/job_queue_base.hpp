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

#include "dectnrp/common/reporting.hpp"
#include "dectnrp/phy/pool/job.hpp"

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

namespace dectnrp::phy {

class job_queue_base_t : public common::reporting_t {
    public:
        explicit job_queue_base_t(const uint32_t id_, const uint32_t capacity_)
            : id(id_),
              capacity(capacity_) {};
        ~job_queue_base_t() = default;

        job_queue_base_t() = delete;
        job_queue_base_t(const job_queue_base_t&) = delete;
        job_queue_base_t& operator=(const job_queue_base_t&) = delete;
        job_queue_base_t(job_queue_base_t&&) = delete;
        job_queue_base_t& operator=(job_queue_base_t&&) = delete;

        /**
         * \brief Function for producers, for instance worker_sync_t. Has no timeout (nto).
         *
         * \param job
         * \return true when was was enqueued
         * \return false if not
         */
        virtual bool enqueue_nto(job_t&& job) = 0;

        void set_permeable() { permeable.store(true, std::memory_order_release); }
        void set_impermeable() { permeable.store(false, std::memory_order_release); }

        const uint32_t id;
        const uint32_t capacity;

    protected:
        virtual std::vector<std::string> report_start() const override = 0;
        virtual std::vector<std::string> report_stop() const override = 0;

        std::atomic<bool> permeable{false};

        int64_t fifo_cnt{0};

        static constexpr uint32_t JOB_QUEUE_WAIT_TIMEOUT_MS{100};

        /**
         * \brief Function for consumers (for instance worker_tx_rx_t). Has a timeout (to).
         *
         * \param job
         * \return true job was filled
         * \return false job was not filled due to a timeout
         */
        virtual bool wait_for_new_job_to(job_t& job) = 0;
};

}  // namespace dectnrp::phy
