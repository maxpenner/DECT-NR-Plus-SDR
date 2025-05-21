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

#include "dectnrp/sections_part2/channel_bandwidth.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp2 {

channel_bandwidth_t get_channel_bandwidth(const uint32_t operating_channel_bandwidth_index) {
    channel_bandwidth_t channel_bandwidth;

    channel_bandwidth.operating_channel_bandwidth_index = operating_channel_bandwidth_index;

    dectnrp_assert(operating_channel_bandwidth_index == 1 ||
                       operating_channel_bandwidth_index == 2 ||
                       operating_channel_bandwidth_index == 3,
                   "Operating channel bandwidth index must be 1, 2 or 3");

    switch (operating_channel_bandwidth_index) {
        case 1:
            channel_bandwidth.nominal_channel_bandwidth = 1728.0;
            channel_bandwidth.transmission_channel_bandwidth = 1512.0;
            break;

        case 2:
            channel_bandwidth.nominal_channel_bandwidth = 3456.0;
            channel_bandwidth.transmission_channel_bandwidth = 3024.0;
            break;

        case 3:
        default:
            channel_bandwidth.nominal_channel_bandwidth = 6912.0;
            channel_bandwidth.transmission_channel_bandwidth = 6048.0;
            break;
    }

    channel_bandwidth.nominal_channel_bandwidth = channel_bandwidth.nominal_channel_bandwidth * 1e6;
    channel_bandwidth.transmission_channel_bandwidth =
        channel_bandwidth.transmission_channel_bandwidth * 1e6;

    return channel_bandwidth;
}

}  // namespace dectnrp::sp2
