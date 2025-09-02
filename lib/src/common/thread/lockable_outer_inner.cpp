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

#include "dectnrp/common/thread/lockable_outer_inner.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common {

bool lockable_outer_inner_t::try_lock_outer() {
    if (!outer.exchange(true, std::memory_order_acquire)) {
        dectnrp_assert(!is_inner_locked(), "incorrect lock state");
        ++stats.outer_locked;
        return true;
    }

    return false;
}

void lockable_outer_inner_t::lock_outer() {
    dectnrp_assert(!is_outer_locked(), "incorrect lock state");
    dectnrp_assert(!is_inner_locked(), "incorrect lock state");

    outer.store(true, std::memory_order_release);
    ++stats.outer_locked;
}

void lockable_outer_inner_t::lock_inner() {
    dectnrp_assert(is_outer_locked(), "incorrect lock state");
    dectnrp_assert(!is_inner_locked(), "incorrect lock state");

    inner.store(true, std::memory_order_release);
    ++stats.inner_locked;
}

void lockable_outer_inner_t::unlock_outer() {
    dectnrp_assert(is_outer_locked(), "incorrect lock state");
    dectnrp_assert(!is_inner_locked(), "incorrect lock state");

    ++stats.outer_unlocked;
    outer.store(false, std::memory_order_release);
}

void lockable_outer_inner_t::unlock_inner() {
    dectnrp_assert(is_outer_locked(), "incorrect lock state");
    dectnrp_assert(is_inner_locked(), "incorrect lock state");

    ++stats.inner_unlocked;
    inner.store(false, std::memory_order_release);
}

bool lockable_outer_inner_t::is_outer_locked() const {
    return outer.load(std::memory_order_acquire);
}

bool lockable_outer_inner_t::is_inner_locked() const {
    return inner.load(std::memory_order_acquire);
}

bool lockable_outer_inner_t::is_outer_locked_inner_locked() const {
    if (is_outer_locked() && is_inner_locked()) {
        return true;
    }

    return false;
}

bool lockable_outer_inner_t::is_outer_locked_inner_unlocked() const {
    if (is_outer_locked() && !is_inner_locked()) {
        return true;
    }

    return false;
}

std::string lockable_outer_inner_t::get_stats_as_string() const {
    std::string str;
    str.append(" outer_locked " + std::to_string(stats.outer_locked));
    str.append(" outer_unlocked " + std::to_string(stats.outer_unlocked));
    str.append(" inner_locked " + std::to_string(stats.inner_locked));
    str.append(" inner_unlocked " + std::to_string(stats.inner_unlocked));
    return str;
}

}  // namespace dectnrp::common
