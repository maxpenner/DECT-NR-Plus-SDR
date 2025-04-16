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

#include "dectnrp/radio/radio_config.hpp"

#include <iostream>
#include <limits>

#include "dectnrp/common/json_parse.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::radio {

radio_config_t::radio_config_t(const std::string directory)
    : layer_config_t(directory, "radio.json") {
    // iterate over every JSON key
    for (auto it = json_parsed.begin(); it != json_parsed.end(); ++it) {
        // does the key describe hardware?
        if (it.key().starts_with(hw_config_t::json_log_key)) {
            hw_config_t hw_config;

            hw_config.id = common::jsonparse::extract_id(it.key(), hw_config_t::json_log_key);

            // hw_name determines which fields are required
            hw_config.hw_name = common::jsonparse::read_string(it, "hw_name");

            hw_config.nof_buffer_tx = common::jsonparse::read_int(it, "nof_buffer_tx", 1, 16);

            hw_config.turnaround_time_us =
                common::jsonparse::read_int(it, "turnaround_time_us", 10, 5000);

            hw_config.tx_burst_leading_zero_us =
                common::jsonparse::read_int(it, "tx_burst_leading_zero_us", 0, 500);

            hw_config.tx_time_advance_smpl =
                common::jsonparse::read_int(it, "tx_time_advance_smpl", 0, 200);

            hw_config.rx_prestream_ms = common::jsonparse::read_int(it, "rx_prestream_ms", 0, 5000);

            hw_config.rx_notification_period_us =
                common::jsonparse::read_int(it, "rx_notification_period_us", 0, 5000);

            const auto tx_thread_config_array =
                common::jsonparse::read_int_array(it, "tx_thread_config", 2, 2, 2);
            hw_config.tx_thread_config.prio_offset = tx_thread_config_array[0];
            hw_config.tx_thread_config.cpu_core = tx_thread_config_array[1];

            const auto rx_thread_config_array =
                common::jsonparse::read_int_array(it, "rx_thread_config", 2, 2, 2);
            hw_config.rx_thread_config.prio_offset = rx_thread_config_array[0];
            hw_config.rx_thread_config.cpu_core = rx_thread_config_array[1];

            // simulator specifics
            if (hw_config.hw_name == "simulator") {
                // nothing to do here
            }
            // USRP specific
            else if (hw_config.hw_name == "usrp") {
                hw_config.uspr_args = common::jsonparse::read_string(it, "uspr_args");
                const auto usrp_tx_async_helper_thread_config_array =
                    common::jsonparse::read_int_array(
                        it, "usrp_tx_async_helper_thread_config", 2, 2, 2);
                hw_config.usrp_tx_async_helper_thread_config.prio_offset =
                    usrp_tx_async_helper_thread_config_array[0];
                hw_config.usrp_tx_async_helper_thread_config.cpu_core =
                    usrp_tx_async_helper_thread_config_array[1];
            } else {
                dectnrp_assert_failure("Undefined hardware type {}.", hw_config.hw_name);
            }

            dectnrp_assert(
                hw_config.id == layer_unit_config_vec.size(), "incorrect id {}", hw_config.id);

            // add new config
            layer_unit_config_vec.push_back(hw_config);
        }
        // does the key describe global simulation parameters?
        else if (it.key().starts_with(hw_config_t::json_log_key_simulation)) {
            hw_config_t::sim_samp_rate_lte = common::jsonparse::read_bool(it, "sim_samp_rate_lte");
            hw_config_t::sim_spp_us = common::jsonparse::read_int(it, "sim_spp_us", 50, 500);
            hw_config_t::sim_samp_rate_speed =
                common::jsonparse::read_int(it, "sim_samp_rate_speed", INT32_MIN, INT32_MAX);
            hw_config_t::sim_channel_name_inter =
                common::jsonparse::read_string(it, "sim_channel_name_inter");
            hw_config_t::sim_channel_name_intra =
                common::jsonparse::read_string(it, "sim_channel_name_intra");
            hw_config_t::sim_noise_type = common::jsonparse::read_string(it, "sim_noise_type");
        }
        // key unknown
        else {
            dectnrp_assert_failure("incorrect prefix for key {}", it.key());
        }
    }
}

}  // namespace dectnrp::radio
