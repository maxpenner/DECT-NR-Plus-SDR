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

#include "dectnrp/application/socket/socket_server.hpp"

#include <sys/socket.h>

#include <cstring>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::application::sockets {

socket_server_t::socket_server_t(const uint32_t id,
                                 const common::threads_core_prio_config_t thread_config,
                                 phy::job_queue_t& job_queue,
                                 const std::vector<uint32_t> ports,
                                 const queue_size_t queue_size)
    : app_server_t(id, thread_config, job_queue, ports.size(), queue_size),
      socketx_t(ports) {
    // setup one socket for every UDP ports requested
    for (uint32_t i = 0; i < udp_vec.size(); ++i) {
        // setup socket
        if ((udp_vec[i]->socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            dectnrp_assert_failure("Server failed to init socket.");
        }

        memset(&udp_vec[i]->servaddr, 0, sizeof(udp_vec[i]->servaddr));

        // fill server information
        udp_vec[i]->servaddr.sin_family = AF_INET;
        udp_vec[i]->servaddr.sin_addr.s_addr = INADDR_ANY;
        udp_vec[i]->servaddr.sin_port = htons(udp_vec[i]->port);

        // bind the socket to the server address
        if (bind(udp_vec[i]->socketfd,
                 (const struct sockaddr*)&udp_vec[i]->servaddr,
                 sizeof(udp_vec[i]->servaddr)) < 0) {
            dectnrp_assert_failure("Server failed to bind socket.");
        }

        // add polling structure
        pfds[i].fd = udp_vec[i]->socketfd;
        pfds[i].events = POLLIN;
    }
}

socket_server_t::~socket_server_t() {
    for (uint32_t i = 0; i < udp_vec.size(); ++i) {
        close(udp_vec[i]->socketfd);
    }
};

queue_level_t socket_server_t::get_queue_level_nto(const uint32_t conn_idx,
                                                   const uint32_t n) const {
    return queue_vec.at(conn_idx)->get_queue_level_nto(n);
}

queue_level_t socket_server_t::get_queue_level_try(const uint32_t conn_idx,
                                                   const uint32_t n) const {
    return queue_vec.at(conn_idx)->get_queue_level_try(n);
}

uint32_t socket_server_t::read_nto(const uint32_t conn_idx, uint8_t* dst) {
    return queue_vec.at(conn_idx)->read_nto(dst);
}

uint32_t socket_server_t::read_try(const uint32_t conn_idx, uint8_t* dst) {
    return queue_vec.at(conn_idx)->read_try(dst);
}

ssize_t socket_server_t::read_datagram(const uint32_t conn_idx) {
    return recvfrom(udp_vec[conn_idx]->socketfd,
                    buffer_local,
                    sizeof(buffer_local),
                    MSG_WAITALL,
                    (struct sockaddr*)&cliaddr,
                    &len);
}

bool socket_server_t::filter_ingress_datagram(const uint32_t conn_idx) {
    // nothing to here so far

    return true;
}

}  // namespace dectnrp::application::sockets
