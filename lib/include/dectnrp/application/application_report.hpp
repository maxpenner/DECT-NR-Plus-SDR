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

namespace dectnrp::application {

class application_report_t {
    public:
        explicit application_report_t(const uint32_t conn_idx_,
                                      const uint32_t N_byte_,
                                      const int64_t rx_time_opsys_64_)
            : conn_idx(conn_idx_),
              N_byte(N_byte_),
              rx_time_opsys_64(rx_time_opsys_64_) {};

        /**
         * \brief When data is passed from or to the application layer, this connection index is
         * used as an identifier between the application layer and the upper layers.
         *
         * A connection index can, for instance, represent one of multiple UDP ports that data was
         * received at (app_server_t) or should be transmitted to (app_client_t). A connection index
         * may also represent a single TUN interface, i.e. the connection index is always 0 and the
         * IP address is part of each in- or outgoing datagram. However, it is also possible that
         * the TUN interface demultiplexes the IP addresses and uses one connection index per IP.
         *
         * It is always up to the firmware what a connection index stands for, since the firmware
         * starts the application layer. The first connection index is always 0, and then
         * incremented by 1.
         */
        int32_t conn_idx{-1};

        /// number of new bytes received at the interface represented by the connection index
        uint32_t N_byte{0};

        /// operation system time at reception in ns since when application layer was started
        int64_t rx_time_opsys_64;
};

}  // namespace dectnrp::application
