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

#include <atomic>
#include <cstdint>
#include <string>

namespace dectnrp::common {

class lockable_outer_inner_t {
    public:
        lockable_outer_inner_t() = default;
        virtual ~lockable_outer_inner_t() = default;

        lockable_outer_inner_t(const lockable_outer_inner_t&) = delete;
        lockable_outer_inner_t& operator=(const lockable_outer_inner_t&) = delete;
        lockable_outer_inner_t(lockable_outer_inner_t&&) = delete;
        lockable_outer_inner_t& operator=(lockable_outer_inner_t&&) = delete;

    protected:
        bool try_lock_outer();

        void lock_outer();
        void lock_inner();

        void unlock_outer();
        void unlock_inner();

        bool is_outer_locked() const;
        bool is_inner_locked() const;

        bool is_outer_locked_inner_locked() const;
        bool is_outer_locked_inner_unlocked() const;

        struct stats_t {
                int64_t outer_locked{0};
                int64_t inner_locked{0};
                int64_t outer_unlocked{0};
                int64_t inner_unlocked{0};
        };

        std::string get_stats_as_string() const;

    private:
        std::atomic<bool> outer{false};
        std::atomic<bool> inner{false};

        stats_t stats;
};

}  // namespace dectnrp::common
