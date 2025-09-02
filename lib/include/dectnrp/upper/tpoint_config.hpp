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
#include <string>
#include <vector>

#include "dectnrp/common/thread/threads.hpp"

namespace dectnrp::upper {

struct tpoint_config_t {
        /// identifier used in JSON and log file
        static const std::string json_log_key;

        /// every termination point has a unique program ID starting at 0
        uint32_t id;

        /// which firmware should be loaded?
        std::string firmware_name;

        /**
         * \brief ID with scope limited to firmware. Can be used to load the same firmware but with
         * slightly different behaviour depending on this ID.
         */
        uint32_t firmware_id;

        /// network IDs required by tpoint (used to precalculate scrambling on PHY)
        std::vector<uint32_t> network_ids;

        /// configuration for server
        common::threads_core_prio_config_t application_server_thread_config;

        /// configuration for client
        common::threads_core_prio_config_t application_client_thread_config;
};

}  // namespace dectnrp::upper
