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

#include <netinet/in.h>

#include <memory>

namespace dectnrp::application::sockets {

class udp_t {
    public:
        explicit udp_t(const uint32_t port_)
            : port(port_) {};
        ~udp_t() = default;

        udp_t() = delete;
        udp_t(const udp_t&) = delete;
        udp_t& operator=(const udp_t&) = delete;
        udp_t(udp_t&&) = delete;
        udp_t& operator=(udp_t&&) = delete;

        friend class socket_server_t;
        friend class socket_client_t;

    private:
        const uint32_t port;
        int socketfd;
        struct sockaddr_in servaddr;
};

}  // namespace dectnrp::application::sockets
