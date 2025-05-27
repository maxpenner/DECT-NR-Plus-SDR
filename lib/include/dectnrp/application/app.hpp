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

#include <pthread.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "dectnrp/application/queue/queue.hpp"
#include "dectnrp/application/queue/queue_size.hpp"
#include "dectnrp/common/thread/threads.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/phy/pool/job_queue.hpp"

namespace dectnrp::application {

class app_t {
    public:
        /**
         * \brief Root class for all servers (accept ingress packets from external applications to
         * the SDR) and clients (forward egress packets to external applications from the SDR).
         *
         * \param id_
         * \param thread_config_
         * \param job_queue_
         * \param N_queue same as the number of connections
         * \param queue_size every queue has the same size
         */
        explicit app_t(const uint32_t id_,
                       const common::threads_core_prio_config_t thread_config_,
                       phy::job_queue_t& job_queue_,
                       const uint32_t N_queue,
                       const queue_size_t queue_size);
        virtual ~app_t() = default;

        app_t() = delete;
        app_t(const app_t&) = delete;
        app_t& operator=(const app_t&) = delete;
        app_t(app_t&&) = delete;
        app_t& operator=(app_t&&) = delete;

        static constexpr uint32_t APP_POLL_WAIT_TIMEOUT_MS{100U};

        /// spawns work_thread executing work_spawn(), sc = server client
        void start_sc();

        /// stops work_thread, sc = server client
        void stop_sc();

        /// number of connections
        virtual uint32_t get_n_connections() const = 0;

    protected:
        const uint32_t id;
        const common::threads_core_prio_config_t thread_config;

        /// required to notify lower layers of new data
        phy::job_queue_t& job_queue;

        pthread_t work_thread;
        std::atomic<bool> keep_running;

        /// continuous time since call of start_sc()
        common::watch_t watch_since_start;

        /// local buffer which inheriting classes can use to temporarily buffer write
        uint8_t buffer_local[limits::app_max_queue_datagram_byte];

        /// one queue per connection
        std::vector<std::unique_ptr<queue_t>> queue_vec;

        /// actual work done in work_thread + work_spawn(), sc = server client
        virtual void work_sc() = 0;

        /// start routine called from pthread_create()
        static void* work_spawn(void* app_server_or_client) {
            app_t* calling_instance = reinterpret_cast<app_t*>(app_server_or_client);
            calling_instance->work_sc();
            return nullptr;
        }
};

}  // namespace dectnrp::application
