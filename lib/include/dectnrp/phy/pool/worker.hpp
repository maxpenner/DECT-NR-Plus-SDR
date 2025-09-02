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

#include <pthread.h>

#include <cstdint>
#include <string>
#include <vector>

#include "dectnrp/common/reporting.hpp"
#include "dectnrp/phy/pool/worker_config.hpp"

namespace dectnrp::phy {

class worker_t : public common::reporting_t {
    public:
        explicit worker_t(worker_config_t& worker_config)
            : id(worker_config.id),
              keep_running(worker_config.keep_running),
              hw(worker_config.hw),
              job_queue(worker_config.job_queue),
              worker_pool_config(worker_config.worker_pool_config),
              buffer_tx_pool(*hw.buffer_tx_pool.get()),
              buffer_rx(*hw.buffer_rx.get()) {};
        virtual ~worker_t() = default;

        worker_t() = delete;
        worker_t(const worker_t&) = delete;
        worker_t& operator=(const worker_t&) = delete;
        worker_t(worker_t&&) = delete;
        worker_t& operator=(worker_t&&) = delete;

        static constexpr uint32_t KEEP_RUNNING_POLL_PERIOD_MS = 100;

        friend class worker_pool_t;

    protected:
        virtual void work() = 0;
        virtual std::vector<std::string> report_start() const override = 0;
        virtual std::vector<std::string> report_stop() const override = 0;

        const uint32_t id;
        const std::atomic<bool>& keep_running;
        radio::hw_t& hw;
        job_queue_t& job_queue;
        const worker_pool_config_t& worker_pool_config;

        radio::buffer_tx_pool_t& buffer_tx_pool;  // deduced from hw
        radio::buffer_rx_t& buffer_rx;            // deduced from hw

        pthread_t work_thread;
};

}  // namespace dectnrp::phy
