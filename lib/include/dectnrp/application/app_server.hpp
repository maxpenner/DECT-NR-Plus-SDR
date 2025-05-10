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

#include <poll.h>

#include <cstdint>
#include <limits>
#include <vector>

#include "dectnrp/application/app.hpp"
#include "dectnrp/application/queue/queue_level.hpp"
#include "dectnrp/common/thread/watch.hpp"

namespace dectnrp::application {

class app_server_t : public app_t {
    public:
        explicit app_server_t(const uint32_t id_,
                              const common::threads_core_prio_config_t thread_config_,
                              phy::job_queue_t& job_queue_,
                              const uint32_t N_queue,
                              const queue_size_t queue_size);
        virtual ~app_server_t() = default;

        app_server_t() = delete;
        app_server_t(const app_server_t&) = delete;
        app_server_t& operator=(const app_server_t&) = delete;
        app_server_t(app_server_t&&) = delete;
        app_server_t& operator=(app_server_t&&) = delete;

        virtual uint32_t get_n_connections() const override = 0;

        /**
         * \brief Get vector with current levels in a specific queue. The first element refers to
         * the oldest datagram, which would be returned with the next read.
         *
         * \param conn_idx
         * \param n number of levels requested, can be set to very large number to get full overview
         * \return
         */
        virtual queue_level_t get_queue_level_nto(const uint32_t conn_idx,
                                                  const uint32_t n) const = 0;
        virtual queue_level_t get_queue_level_try(const uint32_t conn_idx,
                                                  const uint32_t n) const = 0;

        virtual uint32_t read_nto(const uint32_t conn_idx, uint8_t* dst) = 0;
        virtual uint32_t read_try(const uint32_t conn_idx, uint8_t* dst) = 0;

        /// call without an argument to disable the creation on jobs
        void set_job_queue_access_protection_ns(const int64_t job_queue_access_protection_ns_64_ =
                                                    std::numeric_limits<int64_t>::max()) {
            job_queue_access_protection_ns_64 = job_queue_access_protection_ns_64_;
        }

    protected:
        void work_sc() override final;

        /// poll multiple file descriptors each representing connections
        std::vector<struct pollfd> pfds;

        /// every deriving class has its own way of reading datagrams
        virtual ssize_t read_datagram(const uint32_t conn_idx) = 0;

        /**
         * \brief Every deriving class must filter ingress datagrams.
         *
         * \param conn_idx
         * \return true to keep datagram
         * \return false to discard datagram
         */
        virtual bool filter_ingress_datagram(const uint32_t conn_idx) = 0;

        /**
         * \brief The app_server_t accepts data from outside. For each individual datagram, it can
         * enqueue one job to notify the other layers of the SDR. To reduce the number of jobs and
         * by that the number of calls of the job queue, we can define a protection time. Two jobs
         * must be separated by this minimum time.
         *
         * By default, this time is set to zero. So we create one job for every incoming datagram.
         * We can set it to a very large value, so that no jobs are created.
         */
        int64_t job_queue_access_protection_ns_64{0};

        // used to monitor job_queue_access_protection_ns, so we use the operating systems clock,
        // not the SDR's
        common::watch_t watch_job_queue_access_protection;

        /// put job in job queue
        void enqueue_job_nto(const uint32_t conn_idx, uint32_t n_written);
};

}  // namespace dectnrp::application
