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

#include <atomic>
#include <cstdint>

#include "dectnrp/phy/pool/irregular_queue.hpp"
#include "dectnrp/phy/pool/job_queue.hpp"
#include "dectnrp/phy/worker_pool_config.hpp"
#include "dectnrp/radio/hw.hpp"

namespace dectnrp::phy {

struct worker_config_t {
        worker_config_t(const uint32_t id_,
                        const std::atomic<bool>& keep_running_,
                        radio::hw_t& hw_,
                        job_queue_t& job_queue_,
                        irregular_queue_t& irregular_queue_,
                        const worker_pool_config_t& worker_pool_config_)
            : id(id_),
              keep_running(keep_running_),
              hw(hw_),
              job_queue(job_queue_),
              irregular_queue(irregular_queue_),
              worker_pool_config(worker_pool_config_) {}

        uint32_t id;
        const std::atomic<bool>& keep_running;
        radio::hw_t& hw;
        job_queue_t& job_queue;
        irregular_queue_t& irregular_queue;
        const worker_pool_config_t& worker_pool_config;
};

}  // namespace dectnrp::phy
