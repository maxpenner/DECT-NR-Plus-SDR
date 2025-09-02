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

/**
 * \brief The use of a spinlock is experimental. We make an additional distinction between two
 * implementations. The choices are pthread and Rigtorp based on atomics.
 *
 * https://rigtorp.se/spinlock/
 *
 * https://www.realworldtech.com/forum/?threadid=189711&curpostid=189723
 * https://www.realworldtech.com/forum/?threadid=189711&curpostid=189752
 */
#define COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
#ifdef COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
#include <pthread.h>
#else
#include <atomic>
#endif

namespace dectnrp::common {

class spinlock_t {
    public:
        spinlock_t();
        ~spinlock_t();

        spinlock_t(const spinlock_t&) = delete;
        spinlock_t& operator=(const spinlock_t&) = delete;
        spinlock_t(spinlock_t&&) = delete;
        spinlock_t& operator=(spinlock_t&&) = delete;

        void lock();
        bool try_lock();
        void unlock();

    private:
#ifdef COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
        pthread_spinlock_t pthread_spinlock;
#else
        std::atomic<bool> atomic_lock;
#endif
};

}  // namespace dectnrp::common
