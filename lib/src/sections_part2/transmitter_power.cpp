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

#include "dectnrp/sections_part2/transmitter_power.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp2 {

maximum_output_power_t get_maximum_output_power(const uint32_t operating_channel_bandwidth,
                                                const uint32_t RD_power_class) {
    maximum_output_power_t maximum_output_power;

    maximum_output_power.operating_channel_bandwidth = operating_channel_bandwidth;
    maximum_output_power.RD_power_class = RD_power_class;

    dectnrp_assert(operating_channel_bandwidth == 1728000 ||
                       operating_channel_bandwidth == 3456000 ||
                       operating_channel_bandwidth == 6912000,
                   "Incorrect operating channel bandwidth.");

    switch (operating_channel_bandwidth) {
        case 1728000:
            maximum_output_power.measurement_bandwidth = 1512000;
            break;

        case 3456000:
            maximum_output_power.measurement_bandwidth = 3024000;
            break;

        case 6912000:
        default:
            maximum_output_power.measurement_bandwidth = 6048000;
    }

    dectnrp_assert(RD_power_class == 1 || RD_power_class == 2 || RD_power_class == 3,
                   "RD power class must be 1, 2 or 3");

    switch (RD_power_class) {
        case 1:
            maximum_output_power.outputer_power_dBm = 23;
            break;

        case 2:
            maximum_output_power.outputer_power_dBm = 19;
            break;

        case 3:
        default:
            maximum_output_power.outputer_power_dBm = 10;
            break;
    }

    maximum_output_power.outputer_power_tolerance_dB = 2;

    return maximum_output_power;
}

minimum_output_power_t get_maximum_output_power(const uint32_t operating_channel_bandwidth) {
    minimum_output_power_t minimum_output_power;

    minimum_output_power.operating_channel_bandwidth = operating_channel_bandwidth;
    minimum_output_power.outputer_power_dBm = -40;

    return minimum_output_power;
}

}  // namespace dectnrp::sp2
