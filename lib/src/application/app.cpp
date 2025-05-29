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

#include "dectnrp/application/app.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"

namespace dectnrp::application {

app_t::app_t(const uint32_t id_,
             const common::threads_core_prio_config_t thread_config_,
             phy::job_queue_t& job_queue_,
             const uint32_t N_queue,
             const queue_size_t queue_size)
    : id(id_),
      thread_config(thread_config_),
      job_queue(job_queue_) {
    keep_running.store(false, std::memory_order_release);

    dectnrp_assert(0 < N_queue, "ill-defined");
    dectnrp_assert(N_queue <= limits::app_max_connections, "ill-defined");
    dectnrp_assert(queue_size.is_valid(), "ill-defined");

    for (uint32_t i = 0; i < N_queue; ++i) {
        queue_vec.push_back(std::make_unique<queue_t>(queue_size));
    }
}

void app_t::start_sc() {
    dectnrp_assert(!keep_running.load(std::memory_order_acquire), "keep_running already true");

    // give thread permission to run
    keep_running.store(true, std::memory_order_release);

    watch_since_start.reset();

    // start thread that internally executes work function of "this"
    if (!common::threads_new_rt_mask_custom(&work_thread, &work_spawn, this, thread_config)) {
        dectnrp_assert_failure("app unable to start work thread");
    }

    dectnrp_log_inf("{}",
                    std::string("THREAD app " + std::to_string(id) + " " +
                                common::get_thread_properties(work_thread, thread_config)));
};

void app_t::stop_sc() {
    // make thread stop execution internally
    keep_running.store(false, std::memory_order_release);

    pthread_join(work_thread, NULL);
};

}  // namespace dectnrp::application
