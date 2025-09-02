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

namespace dectnrp::sp2 {

struct maximum_output_power_t {
        uint32_t operating_channel_bandwidth;
        uint32_t RD_power_class;
        int32_t outputer_power_dBm;
        int32_t outputer_power_tolerance_dB;
        uint32_t measurement_bandwidth;
};

struct minimum_output_power_t {
        uint32_t operating_channel_bandwidth;
        int32_t outputer_power_dBm;
};

maximum_output_power_t get_maximum_output_power(const uint32_t operating_channel_bandwidth,
                                                const uint32_t RD_power_class);

minimum_output_power_t get_maximum_output_power(const uint32_t operating_channel_bandwidth);

}  // namespace dectnrp::sp2
