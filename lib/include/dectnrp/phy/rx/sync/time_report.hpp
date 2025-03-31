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

#include <cmath>
#include <cstdint>
#include <limits>

namespace dectnrp::phy {

class time_report_t {
    public:
        explicit time_report_t(const int64_t chunk_time_end_64_, const int64_t sync_time_last_64_)
            : chunk_time_end_64(chunk_time_end_64_),
              sync_time_last_64(sync_time_last_64_),
              barrier_time_64(std::max(chunk_time_end_64, sync_time_last_64)) {};

        /**
         * \brief Synchronization is processed in chunks by instances of worker_sync_t, typically
         * one to four instances. These instances are also responsible for creating instances of
         * time_report_t and putting them into the job queue. They do so after completely
         * processing their respective chunk, which then have ended at chunk_time_end_64 (ignoring
         * the overlap in between chunks).
         */
        int64_t chunk_time_end_64;

        /**
         * \brief The member sync_time_last_64 is the last fine synchronization point known to the
         * creating instance of worker_sync_t. It is not possible for packets to be synchronized
         * before this time.
         */
        int64_t sync_time_last_64;

        /**
         * \brief The larger of the two numbers above. Once this time becomes available to any
         * firmware, it is guaranteed that no more packets will arrive with synchronization times
         * before barrier_time_64. It is a "barrier" separating past packets from any potential
         * future packets.
         *
         * In terms of tpoint functions, this implies that whenever the function work_pcc() is
         * called, the respective packet will have a fine synchronization time later or equal
         * barrier_time_64.
         */
        int64_t barrier_time_64;
};

}  // namespace dectnrp::phy
