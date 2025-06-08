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

#include "dectnrp/application/application_server.hpp"

namespace dectnrp::application {

application_server_t::application_server_t(const uint32_t id_,
                                           const common::threads_core_prio_config_t thread_config_,
                                           phy::job_queue_t& job_queue_,
                                           const uint32_t N_queue,
                                           const queue_size_t queue_size)
    : application_t(id_, thread_config_, job_queue_, N_queue, queue_size) {
    pfds.resize(N_queue);
}

void application_server_t::work_sc() {
    int pollin_happened, n;
    uint32_t n_written;

    // allow immediate creation of jobs
    watch_job_queue_access_protection.reset();

    while (keep_running.load(std::memory_order_acquire)) {
        const int num_events = poll(pfds.data(), pfds.size(), APP_POLL_WAIT_TIMEOUT_MS);

        // any sockets with events? if not, poll timed out
        if (num_events > 0) {
            for (std::size_t i = 0; i < pfds.size(); ++i) {
                pollin_happened = pfds[i].revents & POLLIN;

                if (pollin_happened) {
                    n = read_datagram(i);

                    if (!filter_ingress_datagram(i)) {
                        continue;
                    }

                    // if a new datagram was received ...
                    if (n > 0) {
                        // ... get a lock on the queue and try to write the datagram
                        n_written = queue_vec[i]->write_nto((const uint8_t*)buffer_local, n);

                        // if we successfully wrote the datagram to the queue ...
                        if (n_written > 0) {
                            /// ... create a job in the job queue for quick processing
                            enqueue_job_nto(i, n_written);
                        }
                    }

                    // ... otherwise discard the datagram
                    // nothing to do here
                }
            }
        }
    }
};

void application_server_t::enqueue_job_nto(const uint32_t conn_idx, uint32_t n_written) {
    if (watch_job_queue_access_protection.get_elapsed() >= job_queue_access_protection_ns_64) {
        job_queue.enqueue_nto(phy::job_t(application::application_report_t(
            conn_idx, n_written, watch_since_start.get_elapsed())));

        watch_job_queue_access_protection.reset();
    }
}

}  // namespace dectnrp::application
