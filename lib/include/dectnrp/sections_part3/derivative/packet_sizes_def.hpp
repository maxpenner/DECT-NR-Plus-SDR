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

#include <cstdint>

namespace dectnrp::section3 {

using packet_sizes_def_t = struct packet_sizes_def_t {
        /// freely selected during operation (max values defined by radio device class)
        uint32_t u{};
        uint32_t b{};

        /// freely selected during operation (max value for PacketLength is 16 (4 bit value + 1))
        uint32_t PacketLengthType{};
        uint32_t PacketLength{};

        /// freely selected during operation (max values defined by number of antennas of radio
        /// device class)
        uint32_t tm_mode_index{};

        /// freely selected during operation (max value defined by radio device class)
        uint32_t mcs_index{};

        /// freely selected during operation (max values defined by radio device class)
        uint32_t Z{};
};

}  // namespace dectnrp::section3
