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

#include <memory>

#include "dectnrp/phy/pool/baton.hpp"
#include "dectnrp/phy/pool/irregular.hpp"
#include "dectnrp/phy/pool/worker.hpp"
#include "dectnrp/phy/rx/sync/sync_chunk.hpp"

namespace dectnrp::phy {

class worker_sync_t final : public worker_t {
    public:
        explicit worker_sync_t(worker_config_t& worker_config,
                               baton_t& baton_,
                               irregular_t& irregular_);
        ~worker_sync_t() = default;

        worker_sync_t() = delete;
        worker_sync_t(const worker_sync_t&) = delete;
        worker_sync_t& operator=(const worker_sync_t&) = delete;
        worker_sync_t(worker_sync_t&&) = delete;
        worker_sync_t& operator=(worker_sync_t&&) = delete;

        friend class worker_pool_t;

    private:
        void work() override final;
        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

        baton_t& baton;
        irregular_t& irregular;

        std::unique_ptr<sync_chunk_t> sync_chunk;

        /**
         * \brief Once all instances of worker_sync_t are ready to process, the radio layer
         * immediately starts sending samples at the hardware sample rate. This can overwhelm the
         * synchronization and trigger an assert because of a backlog of samples in this initial
         * phase. As a countermeasure, we process incoming samples only partially for a warmup
         * period.
         */
        void warmup();

        static constexpr std::size_t warmup_sec{1};

        void irregular_callback(const int64_t now_64);

        void enqueue_job_nto(const sync_report_t& sync_report);

        struct stats_t {
                int64_t job_regular{0};
                int64_t job_packet{0};
                int64_t job_packet_not_unique{0};
                int64_t job_packet_no_slot{0};
        } stats;
};

}  // namespace dectnrp::phy
