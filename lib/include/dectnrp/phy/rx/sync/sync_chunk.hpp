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

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "dectnrp/phy/rx/rx_pacer.hpp"
#include "dectnrp/phy/rx/sync/autocorrelator_detection.hpp"
#include "dectnrp/phy/rx/sync/autocorrelator_peak.hpp"
#include "dectnrp/phy/rx/sync/crosscorrelator.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"
#include "dectnrp/phy/worker_pool_config.hpp"

namespace dectnrp::phy {

class sync_chunk_t final : public rx_pacer_t {
    public:
        typedef std::function<void(int64_t)> enqueue_irregular_job_if_due_cb_t;

        explicit sync_chunk_t(const radio::buffer_rx_t& buffer_rx_,
                              const worker_pool_config_t& worker_pool_config_,
                              const uint32_t chunk_length_samples_,
                              const uint32_t chunk_stride_samples_,
                              const uint32_t chunk_offset_samples_,
                              const uint32_t ant_streams_unit_length_samples_,
                              enqueue_irregular_job_if_due_cb_t enqueue_irregular_job_if_due_cb_);
        ~sync_chunk_t() = default;

        sync_chunk_t() = delete;
        sync_chunk_t(const sync_chunk_t&) = delete;
        sync_chunk_t& operator=(const sync_chunk_t&) = delete;
        sync_chunk_t(sync_chunk_t&&) = delete;
        sync_chunk_t& operator=(sync_chunk_t&&) = delete;

        /**
         * \brief Called by worker_sync to set the initial search time upon startup, which is not
         * necessarily zero if hardware sample count is ahead. Also, this function waits for the
         * first chunk to arrive.
         *
         * \param search_time_start_64_ global synchronization start time
         */
        void wait_for_first_chunk_nto(const int64_t search_time_start_64_);

        /**
         * \brief Returns a sync_report_t when a packet was found in the current chunk's search
         * range. Returns an empty optional when chunk was fully processed without detecting a
         * packet.
         *
         * \return
         */
        std::optional<sync_report_t> search();

        /// returns true if the full search range was covered
        bool is_chunk_completely_processed();

        /// start time for current chunk as index of first sample
        int64_t get_chunk_time_start() const;

        /// ending time of chunk without overlapping area
        int64_t get_chunk_time_end() const;

        const uint32_t chunk_length_samples;
        const uint32_t chunk_stride_samples;
        const uint32_t chunk_offset_samples;

        struct stats_t {
                int64_t waitings_for_chunk{0};
                int64_t detections{0};
                int64_t coarse_peaks{0};
                int64_t chunk_processed_without_detection_at_the_end{0};
        };

        stats_t get_stats() const { return stats; };

    private:
        /// all these values refer to the maximum value of the radio device class
        const uint32_t u_max;
        const uint32_t stf_nof_pattern;

        /**
         * \brief An STF, critically sampled and with b=1, has a length of either 112 (u=1) or 144
         * (u=2,4,8) samples. Its length in samples is increased by b*oversampling, e.g. with b=12
         * and os=2 the shorter length becomes 112*12*2 = 2688 samples.
         */
        const uint32_t bos_fac;
        const uint32_t stf_bos_length_samples;
        const uint32_t stf_bos_pattern_length_samples;

        /* The value of chunk_length_samples refers to the hardware sample rate and it is always a
         * multiple of L. A is the chunk length in samples after resampling at the DECT NR+ sample
         * rate, and B is an additional overlap region between chunks. The autocorrelator for
         * detection must search at least until the end of B, but not beyond C. The autocorrelator
         * for peak search may not search beyond D. The pacer must output at least D samples.
         *
         *               A after resampling                B     C    D
         *    |_______________________________________|_________|_|_______|
         *                                                        |       |
         *                                                        |       |
         *         autocorrelator can detect packets up to this point     |
         *                                                                |
         *                         coarse peaks can be found up to this point
         */
        const uint32_t A, B, C, D;

        enqueue_irregular_job_if_due_cb_t enqueue_irregular_job_if_due_cb;

        /// internal time keeping
        int64_t chunk_time_start_64;
        void set_next_chunk();
        void wait_for_chunk_nto();

        /// units for running synchronization algorithms
        std::unique_ptr<autocorrelator_detection_t> autocorrelator_detection;
        std::unique_ptr<autocorrelator_peak_t> autocorrelator_peak;
        std::unique_ptr<crosscorrelator_t> crosscorrelator;

        stats_t stats;
};

}  // namespace dectnrp::phy
