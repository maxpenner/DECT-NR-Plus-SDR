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

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>

// #define PHY_POOL_BATON_USES_CONDITION_VARIABLE_OR_BUSYWAITING
#ifdef PHY_POOL_BATON_USES_CONDITION_VARIABLE_OR_BUSYWAITING
#else
#include <atomic>
#endif

#include "dectnrp/phy/pool/token.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::phy {

class baton_t {
    public:
        explicit baton_t(const uint32_t nof_worker_sync_,
                         const int64_t sync_time_unique_limit_64_,
                         const uint32_t job_regular_period_);
        ~baton_t() = default;

        baton_t() = delete;
        baton_t(const baton_t&) = delete;
        baton_t& operator=(const baton_t&) = delete;
        baton_t(baton_t&&) = delete;
        baton_t& operator=(baton_t&&) = delete;

        static constexpr uint32_t BATON_WAIT_TIMEOUT_MS = 100;

        /**
         * \brief Called by worker_pool_t with its associated tpoint and token. This class with use
         * that information to call the tpoint function work_start_imminent().
         *
         * \param tpoint_
         * \param token_
         */
        void set_tpoint_to_notify(upper::tpoint_t* tpoint_, std::shared_ptr<token_t> token_);

        /**
         * \brief Called by every worker_sync_t to find a common start time for synchronization,
         * thread-safe with no timeout (nto).
         *
         * \param now_64 suggestion of each worker_sync_t instance
         * \return
         */
        int64_t register_and_wait_for_others_nto(const int64_t now_64);

        /// thread-safe
        bool is_id_holder_the_same(const uint32_t id_caller) const;
        bool wait_to(const uint32_t id_target);
        void pass_on(const uint32_t id_caller);

        /// not thread-safe, call only when holding the baton
        bool is_sync_time_unique(const int64_t sync_time_candidate_64);
        bool is_job_regular_due();
        int64_t get_sync_time_last() const { return sync_time_last_64; };

#ifdef ASSERT_ENABLED
        int64_t chunk_time_end_64;
#endif

    private:
        const uint32_t nof_worker_sync;
        const int64_t sync_time_unique_limit_64;
        const uint32_t job_regular_period;

        // registration
        std::mutex register_mtx;
        std::condition_variable register_cv;
        uint32_t register_cnt;
        int64_t register_now_64;

        // post-registration
        upper::tpoint_t* tpoint;
        std::shared_ptr<token_t> token;

#ifdef PHY_POOL_BATON_USES_CONDITION_VARIABLE_OR_BUSYWAITING
        mutable std::mutex mtx;
        std::condition_variable cv;
        uint32_t id_holder;
#else
        std::atomic<uint32_t> id_holder;
#endif

        /// not thread-safe, write and read only when holding the baton
        int64_t sync_time_last_64;
        uint32_t job_regular_period_cnt;
};

}  // namespace dectnrp::phy
