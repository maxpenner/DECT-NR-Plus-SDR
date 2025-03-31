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

#include "dectnrp/application/app_server.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"

namespace dectnrp::application {

app_server_t::app_server_t(const uint32_t id_,
                           const common::threads_core_prio_config_t thread_config_,
                           phy::job_queue_t& job_queue_,
                           const uint32_t n_connections_,
                           const uint32_t n_item_byte_max_)
    : app_t(id_, thread_config_, job_queue_, n_connections_, n_item_byte_max_) {}

void app_server_t::enqueue_job_nto(const uint32_t conn_idx, uint32_t n_written) {
    if (watch_job_queue_access_protection.get_elapsed() >= job_queue_access_protection_ns_64) {
        job_queue.enqueue_nto(phy::job_t(
            upper::upper_report_t(conn_idx, n_written, watch_since_start.get_elapsed())));

        watch_job_queue_access_protection.reset();
    }
}

}  // namespace dectnrp::application
