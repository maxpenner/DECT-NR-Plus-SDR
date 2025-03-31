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

socket_server_t::socket_server_t(const uint32_t id_,
                                 const common::threads_core_prio_config_t thread_config_,
                                 phy::job_queue_t& job_queue_,
                                 const std::vector<uint32_t> ports_,
                                 const uint32_t N_item_,
                                 const uint32_t N_item_byte_)
    : app_server_t(id_, thread_config_, job_queue_, ports_.size(), N_item_byte_),
      socketx_t(ports_, N_item_, N_item_byte_) {
    // setup one socket for every socket_items_pair defined
    for (uint32_t i = 0; i < socket_items_pair_vec.size(); ++i) {
        // setup socket
        if ((socket_items_pair_vec[i]->socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            dectnrp_assert_failure("Server failed to init socket.");
        }

        memset(&socket_items_pair_vec[i]->servaddr, 0, sizeof(socket_items_pair_vec[i]->servaddr));

        // fill server information
        socket_items_pair_vec[i]->servaddr.sin_family = AF_INET;
        socket_items_pair_vec[i]->servaddr.sin_addr.s_addr = INADDR_ANY;
        socket_items_pair_vec[i]->servaddr.sin_port = htons(socket_items_pair_vec[i]->port);

        // bind the socket to the server address
        if (bind(socket_items_pair_vec[i]->socketfd,
                 (const struct sockaddr*)&socket_items_pair_vec[i]->servaddr,
                 sizeof(socket_items_pair_vec[i]->servaddr)) < 0) {
            dectnrp_assert_failure("Server failed to bind socket.");
        }
    }
}

socket_server_t::~socket_server_t() {
    for (uint32_t i = 0; i < socket_items_pair_vec.size(); ++i) {
        close(socket_items_pair_vec[i]->socketfd);
    }
};

void socket_server_t::work_sc() {
    const int __nfds = socket_items_pair_vec.size();

    // create the poll structure required
    std::vector<struct pollfd> pfds(__nfds);
    for (int i = 0; i < __nfds; ++i) {
        pfds[i].fd = socket_items_pair_vec[i]->socketfd;
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
                    n = recvfrom(socket_items_pair_vec[i]->socketfd,
                                 buffer_local,
                                 APP_LOCALBUFFER_BYTE,
                                 MSG_WAITALL,
                                 (struct sockaddr*)&cliaddr,
                                 &len);

                    // if a new UDP datagram was received ...
                    if (n > 0) {
                        // ... get a lock on the socket_items_pair and try to write the UDP datagram
                        // payload to it
                        n_written = socket_items_pair_vec[i]->items->write_nto(
                            (const uint8_t*)buffer_local, n);

                        // if we successfully wrote the datagram to items ...
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

items_level_report_t socket_server_t::get_items_level_report_nto(const uint32_t conn_idx,
                                                                 const uint32_t n) const {
    return socket_items_pair_vec[conn_idx]->items->get_items_level_report_nto(n);
}

items_level_report_t socket_server_t::get_items_level_report_try(const uint32_t conn_idx,
                                                                 const uint32_t n) const {
    return socket_items_pair_vec[conn_idx]->items->get_items_level_report_try(n);
}

uint32_t socket_server_t::read_nto(const uint32_t conn_idx, uint8_t* dst) {
    return socket_items_pair_vec[conn_idx]->items->read_nto(dst);
}

uint32_t socket_server_t::read_try(const uint32_t conn_idx, uint8_t* dst) {
    return socket_items_pair_vec[conn_idx]->items->read_try(dst);
}

}  // namespace dectnrp::application::sockets
