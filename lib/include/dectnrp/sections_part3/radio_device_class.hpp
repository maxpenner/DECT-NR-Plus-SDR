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

namespace dectnrp::sp3 {

struct radio_device_class_t {
        std::string radio_device_class_string{};

        /// min refers to minimum that is supported
        uint32_t u_min;
        uint32_t b_min;
        uint32_t N_TX_min;
        uint32_t mcs_index_min;
        uint32_t M_DL_HARQ_min;
        uint32_t M_connection_DL_HARQ_min;
        uint32_t N_soft_min;
        uint32_t Z_min;

        /**
         * \brief According to the standard, the maximum PacketLength is 16. PLCF contains a 4 bit
         * field, interpreted as "value+1", so 15+1=16 is the maximum. However, in order to minimize
         * the amount of memory that is preallocated, we introduce this limit.
         */
        uint32_t PacketLength_min;
};

radio_device_class_t get_radio_device_class(const std::string& radio_device_class_string);

}  // namespace dectnrp::sp3
