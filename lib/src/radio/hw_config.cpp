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

#include "dectnrp/radio/hw_config.hpp"

namespace dectnrp::radio {

const std::string hw_config_t::json_log_key = "HW";
const std::string hw_config_t::json_log_key_simulation = "SIMULATION";

bool hw_config_t::sim_samp_rate_lte;
uint32_t hw_config_t::sim_spp_us;
int32_t hw_config_t::sim_samp_rate_speed;
std::string hw_config_t::sim_channel_name_inter;
std::string hw_config_t::sim_channel_name_intra;
std::string hw_config_t::sim_noise_type;

}  // namespace dectnrp::radio
