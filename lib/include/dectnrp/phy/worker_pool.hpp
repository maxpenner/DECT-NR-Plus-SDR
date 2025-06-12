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
#include <memory>
#include <vector>

#include "dectnrp/common/json/json_export.hpp"
#include "dectnrp/common/json/json_switch.hpp"
#include "dectnrp/common/layer/layer_unit.hpp"
#include "dectnrp/phy/interfaces/layers_downwards/phy_radio.hpp"
#include "dectnrp/phy/pool/baton.hpp"
#include "dectnrp/phy/pool/irregular.hpp"
#include "dectnrp/phy/pool/job_queue.hpp"
#include "dectnrp/phy/pool/token.hpp"
#include "dectnrp/phy/pool/worker_sync.hpp"
#include "dectnrp/phy/pool/worker_tx_rx.hpp"
#include "dectnrp/phy/worker_pool_config.hpp"
#include "dectnrp/radio/hw.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::phy {

class worker_pool_t final : public common::layer_unit_t {
    public:
        explicit worker_pool_t(const worker_pool_config_t& worker_pool_config_,
                               radio::hw_t& hw_,
                               phy_radio_t& phy_radio_);
        ~worker_pool_t() = default;

        worker_pool_t() = delete;
        worker_pool_t(const worker_pool_t&) = delete;
        worker_pool_t& operator=(const worker_pool_t&) = delete;
        worker_pool_t(worker_pool_t&&) = delete;
        worker_pool_t& operator=(worker_pool_t&&) = delete;

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
        static constexpr uint32_t JSON_export_minimum_number_of_packets{100};
#endif

        /**
         * \brief Each worker pool is associated with a tpoint. Pointers to tpoints are set by upper
         * layer during runtime, and only after Radio layer and PHY were initialized. This pointer
         * will be used similarly to a callback function.
         *
         * \param tpoint_ tpoint associated with this pool
         * \param token_ token to use when locking tpoint
         * \param token_call_id_ ID to use when locking token
         */
        void configure_tpoint_calls(upper::tpoint_t* tpoint_,
                                    std::shared_ptr<token_t> token_,
                                    const uint32_t token_call_id_);

        /**
         * \brief Threads of upper layers can also post jobs on the queue, e.g. when new data
         * arrives. Jobs are then processed by the powerful workers.
         *
         * \return
         */
        job_queue_t& get_job_queue() const { return *job_queue.get(); };

        /**
         * \brief Network IDs are used for scrambling on PHY. All conceivable scrambling sequence
         * should be precalculated, otherwise timing of the SDR can hiccup. This function populates
         * new network IDs amongst all workers.
         *
         * \param network_id
         */
        void add_network_id(const uint32_t network_id);

        /**
         * \brief Threads must be started after construction because upper must give each
         * worker_pool_t a reference to its tpoint_t.
         */
        void start_threads_and_get_ready_to_process_iq_samples();

    private:
        const worker_pool_config_t worker_pool_config;

        void work_stop() override final;

        std::atomic<bool> keep_running;

        /// job queue: filled by worker_sync_t, read by worker_tx_rx
        std::unique_ptr<job_queue_t> job_queue;

        /// export data in real-time
        std::unique_ptr<common::json_export_t> json_export;

        /// worker for tx and rx that call tpoint, consumers of jobs
        std::vector<std::unique_ptr<worker_tx_rx_t>> worker_tx_rx_vec;

        /**
         * \brief The baton is used to coordinate access of worker_sync_t instances to the job_queue
         * and to avoid double detection. Every instance of worker_sync_t has a reference to it. Any
         * worker_sync_t instance is only allowed to access the job queue under the following
         * conditions:
         *
         * 1. The current baton_holder_id has the value of the own id. It is initialized with 0.
         * 2. The sync time from a current sync_report_t is not too close to the baton time.
         */
        std::unique_ptr<baton_t> baton;

        irregular_t irregular;

        std::vector<std::unique_ptr<worker_sync_t>> worker_sync_vec;

        /// check whether sync parameters are set correctly
        void check_sync_param() const;

        /// check whether timing of synchronization is possible
        void check_sync_timing() const;

        /// determine the maximum number of samples of deviation
        int64_t get_sync_time_unique_limit() const;

        static void* work_spawn(void* worker) {
            worker_t* calling_instance = reinterpret_cast<worker_t*>(worker);
            calling_instance->work();
            return nullptr;
        }
};

}  // namespace dectnrp::phy
