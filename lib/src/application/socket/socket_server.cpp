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

#include <arpa/inet.h>   // sockaddr_in
#include <poll.h>        // poll
#include <sys/socket.h>  // socket

#include <cstring>  // memset

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
    }
}

socket_server_t::~socket_server_t() {
    for (uint32_t i = 0; i < udp_vec.size(); ++i) {
        close(udp_vec[i]->socketfd);
    }
};

void socket_server_t::work_sc() {
    const int __nfds = udp_vec.size();

    // create the poll structure required
    std::vector<struct pollfd> pfds(__nfds);
    for (int i = 0; i < __nfds; ++i) {
        pfds[i].fd = udp_vec[i]->socketfd;
        pfds[i].events = POLLIN;
    }

    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    int pollin_happened, n;
    uint32_t n_written;

    // allow immediate creation of jobs
    watch_job_queue_access_protection.reset();

    while (keep_running.load(std::memory_order_acquire)) {
        // poll all sockets
        const int num_events = poll(pfds.data(), __nfds, APP_POLL_WAIT_TIMEOUT_MS);

        // any sockets with events? if not, poll timed out
        if (num_events > 0) {
            for (int i = 0; i < __nfds; ++i) {
                pollin_happened = pfds[i].revents & POLLIN;

                if (pollin_happened) {
                    n = recvfrom(udp_vec[i]->socketfd,
                                 buffer_local,
                                 sizeof(buffer_local),
                                 MSG_WAITALL,
                                 (struct sockaddr*)&cliaddr,
                                 &len);

                    // if a new UDP datagram was received ...
                    if (n > 0) {
                        // ... get a lock on the queue, try to write the UDP datagram payload to it
                        n_written = queue_vec[i]->write_nto((const uint8_t*)buffer_local, n);

                        // if we successfully wrote the datagram to the queue ...
                        if (n_written > 0) {
                            /// ... create a job in the job queue for quick processing
                            enqueue_job_nto(i, n_written);
                        }
                    }

                    // ... otherwise discard the UDP datagram payload
                    // nothing to do here
                }
            }
        }
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

}  // namespace dectnrp::application::sockets
