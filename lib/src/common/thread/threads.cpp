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

#include "dectnrp/common/thread/threads.hpp"

#include <thread>

#include "dectnrp/common/prog/assert.hpp"

#ifdef NDEBUG
#else
#include "dectnrp/common/prog/log.hpp"
#endif

#define DEFAULT_PRIORITY_OFFSET 60
#define REAL_TIME_POLICY SCHED_FIFO  // SCHED_RR

namespace dectnrp::common {

static bool threads_new_rt_cpu_custom(
    pthread_t* thread, void* (*start_routine)(void*), void* arg, int cpu_core, int prio_offset) {
    bool ret = false;

    pthread_attr_t attr;
    struct sched_param param;
    cpu_set_t cpuset;
    bool attr_enable = false;

    if (prio_offset >= 0) {
        param.sched_priority = sched_get_priority_max(REAL_TIME_POLICY) - prio_offset;
        pthread_attr_init(&attr);
        if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
            perror("pthread_attr_setinheritsched");
        }
        if (pthread_attr_setschedpolicy(&attr, REAL_TIME_POLICY)) {
            perror("pthread_attr_setschedpolicy");
        }
        if (pthread_attr_setschedparam(&attr, &param)) {
            perror("pthread_attr_setschedparam");
            dectnrp_assert_failure("Error not enough privileges to set Scheduling priority");
        }
        attr_enable = true;
    }

    if (cpu_core >= 0) {
        CPU_ZERO(&cpuset);
        CPU_SET((size_t)cpu_core, &cpuset);

        if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset)) {
            perror("pthread_attr_setaffinity_np");
        }
    }

// TSAN seems to have issues with thread attributes when running as normal user, disable them in
// that case
#if HAVE_TSAN
    attr_enable = false;
#endif

    int err = pthread_create(thread, attr_enable ? &attr : NULL, start_routine, arg);

    dectnrp_assert(err == 0, "pthread_create() failed");

    if (err) {
        if (EPERM == err) {
            // Join failed thread for avoiding memory leak from previous trial
            pthread_join(*thread, NULL);
            perror("Warning: Failed to create thread with real-time priority");
        } else {
            perror("pthread_create() failed");
        }
    } else {
        ret = true;
    }

    if (attr_enable) {
        pthread_attr_destroy(&attr);
    }

    return ret;
}

bool threads_new_rt_mask_custom(pthread_t* thread,
                                void* (*start_routine)(void*),
                                void* arg,
                                threads_core_prio_config_t threads_core_prio_config) {
    int prio_offset = threads_core_prio_config.prio_offset;
    int cpu_core = threads_core_prio_config.cpu_core;

#ifdef NDEBUG
#else
    prio_offset = -1;
    cpu_core = -1;
    dectnrp_log_wrn("Detected build variant Debug. Overwriting prio_offset and cpu_core.");
#endif

    bool ret = false;

    if (prio_offset > sched_get_priority_max(REAL_TIME_POLICY)) {
        prio_offset = sched_get_priority_max(REAL_TIME_POLICY);
    }

    const int n_cpu_cores = std::thread::hardware_concurrency();

    dectnrp_assert(cpu_core < n_cpu_cores,
                   "CPU core picked {} exceeds number of cores in system {}",
                   cpu_core,
                   n_cpu_cores);

    // we multiply mask by 100 to distinguish it from a single cpu core id
    ret = threads_new_rt_cpu_custom(thread, start_routine, arg, cpu_core, prio_offset);

    return ret;
}

std::string get_thread_properties(const pthread_t& thread,
                                  const threads_core_prio_config_t& threads_core_prio_config) {
    int policy;
    struct sched_param param;
    pthread_getschedparam(thread, &policy, &param);

    std::string prop = "Policy=" + std::to_string(policy) +
                       " Priority=" + std::to_string(param.sched_priority) +
                       " Core=" + std::to_string(threads_core_prio_config.cpu_core);

    return prop;
}

}  // namespace dectnrp::common
