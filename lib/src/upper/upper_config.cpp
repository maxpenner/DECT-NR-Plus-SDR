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

#include "dectnrp/upper/upper_config.hpp"

#include "dectnrp/common/json/json_parse.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::upper {

upper_config_t::upper_config_t(const std::string directory)
    : layer_config_t(directory, "upper.json") {
    // iterate over every upper configuration
    for (auto it = json_parsed.begin(); it != json_parsed.end(); ++it) {
        dectnrp_assert(it.key().starts_with(tpoint_config_t::json_log_key),
                       "incorrect prefix for key {}",
                       it.key());

        tpoint_config_t tpoint_config;

        tpoint_config.id = common::jsonparse::extract_id(it.key(), tpoint_config_t::json_log_key);

        tpoint_config.firmware_name = common::jsonparse::read_string(it, "firmware_name");

        tpoint_config.firmware_id = common::jsonparse::read_int(it, "firmware_id", 0, 999);

        const auto network_ids_array =
            common::jsonparse::read_int_array(it, "network_ids", 1, 10, 1);
        for (auto network_id : network_ids_array) {
            tpoint_config.network_ids.push_back(network_id);
        }

        const auto app_server_thread_config =
            common::jsonparse::read_int_array(it, "app_server_thread_config", 2, 2, 2);
        tpoint_config.app_server_thread_config.prio_offset = app_server_thread_config[0];
        tpoint_config.app_server_thread_config.cpu_core = app_server_thread_config[1];

        const auto app_client_thread_config =
            common::jsonparse::read_int_array(it, "app_client_thread_config", 2, 2, 2);
        tpoint_config.app_client_thread_config.prio_offset = app_client_thread_config[0];
        tpoint_config.app_client_thread_config.cpu_core = app_client_thread_config[1];

        dectnrp_assert(
            tpoint_config.id == layer_unit_config_vec.size(), "incorrect id {}", tpoint_config.id);

        layer_unit_config_vec.push_back(tpoint_config);
    }
}

}  // namespace dectnrp::upper
