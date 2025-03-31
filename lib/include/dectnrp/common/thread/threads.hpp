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

#include <string>

namespace dectnrp::common {

/* prio_offset: If set to 0 to 99, thread priority is 99 - prio_offset, i.e. maximum priority it set
 * for 0 and minimum priority for 99. Scheduling is SCHED_FIFO (or SCHED_RR, see threads.cpp). If
 * smaller 0, scheduler picks priority.
 *
 * cpu_core: If set to value between 0 and n_cores-1, thread will start on respective core. If
 * smaller 0, scheduler picks core.
 */
using threads_core_prio_config_t = struct threads_core_prio_config_t {
        int prio_offset{-1};
        int cpu_core{-1};
};

bool threads_new_rt_mask_custom(pthread_t* thread,
                                void* (*start_routine)(void*),
                                void* arg,
                                threads_core_prio_config_t threads_core_prio_config);

std::string get_thread_properties(const pthread_t& thread,
                                  const threads_core_prio_config_t& threads_core_prio_config);

}  // namespace dectnrp::common
