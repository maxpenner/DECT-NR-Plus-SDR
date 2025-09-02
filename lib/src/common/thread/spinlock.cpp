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

#include "dectnrp/common/thread/spinlock.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common {

spinlock_t::spinlock_t() {
#ifdef COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
    if (pthread_spin_init(&pthread_spinlock, 0)) {
        dectnrp_assert_failure("Spinlock init failed.");
    }
#else
    unlock();
#endif
}

spinlock_t::~spinlock_t() {
#ifdef COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
    pthread_spin_destroy(&pthread_spinlock);
#else
#endif
}

void spinlock_t::lock() {
#ifdef COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
    pthread_spin_lock(&pthread_spinlock);
#else
    while (1) {
        if (!atomic_lock.exchange(true, std::memory_order_acquire)) {
            break;
        }

        while (atomic_lock.load(std::memory_order_relaxed)) {
            //__builtin_ia32_pause();
        }
    }
#endif
}

bool spinlock_t::try_lock() {
#ifdef COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
    return pthread_spin_trylock(&pthread_spinlock) == 0;
#else
    /* source: https://rigtorp.se/spinlock/
     * First do a relaxed load to check if lock is free in order to prevent unnecessary cache misses
     * if someone does while(!try_lock()).
     */
    return !atomic_lock.load(std::memory_order_relaxed) &&
           !atomic_lock.exchange(true, std::memory_order_acquire);
#endif
}

void spinlock_t::unlock() {
#ifdef COMMON_THREADS_SPINLOCK_USES_PTHREAD_SPINLOCK_OR_ATOMIC
    pthread_spin_unlock(&pthread_spinlock);
#else
    atomic_lock.store(false, std::memory_order_release);
#endif
}

}  // namespace dectnrp::common
