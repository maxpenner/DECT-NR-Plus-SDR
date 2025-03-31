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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "dectnrp/common/thread/watch.hpp"

namespace dectnrp::application {

class udp_t {
    public:
        udp_t() = default;
        ~udp_t();

        std::size_t add_connection_tx(const std::string ip, const uint32_t port);
        std::size_t add_connection_rx(const std::string ip,
                                      const uint32_t port,
                                      const std::size_t timeout_us);

        void rm_connection_tx(const std::size_t idx);
        void rm_connection_rx(const std::size_t idx);

        ssize_t tx(const std::size_t idx, const uint8_t* buffer, const std::size_t buffer_len);
        ssize_t rx(const std::size_t idx, const uint8_t* buffer, const std::size_t buffer_len);

        template <typename Res = common::micro, typename Clock = common::utc_clock>
        ssize_t tx_timed(const std::size_t idx,
                         const uint8_t* buffer,
                         const std::size_t buffer_len,
                         const int64_t tx_time_64) {
            if (!common::watch_t::sleep_until<Res, Clock>(tx_time_64)) {
                return -1;
            }

            return tx(idx, buffer, buffer_len);
        }

    private:
        struct connection_t {
                int socked_fd;
                struct sockaddr_in sa_in;
        };

        std::vector<connection_t> conn_tx;
        std::vector<connection_t> conn_rx;
};

}  // namespace dectnrp::application
