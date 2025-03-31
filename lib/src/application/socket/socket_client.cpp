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

#include "dectnrp/application/socket/socket_client.hpp"

#include <arpa/inet.h>   // sockaddr_in
#include <sys/socket.h>  // socket

#include <cstring>  // memset

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::application::sockets {

socket_client_t::socket_client_t(const uint32_t id_,
                                 const common::threads_core_prio_config_t thread_config_,
                                 phy::job_queue_t& job_queue_,
                                 const std::vector<uint32_t> ports_,
                                 const uint32_t N_item_,
                                 const uint32_t N_item_byte_)
    : app_client_t(id_, thread_config_, job_queue_, ports_.size(), N_item_byte_),
      socketx_t(ports_, N_item_, N_item_byte_) {
    // setup one socket for every socket_items_pair defined
    for (uint32_t i = 0; i < socket_items_pair_vec.size(); ++i) {
        // setup socket
        if ((socket_items_pair_vec[i]->socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            dectnrp_assert_failure("Server failed to init socket.");
        }

        memset(&socket_items_pair_vec[i]->servaddr, 0, sizeof(socket_items_pair_vec[i]->servaddr));

        // filling server information
        socket_items_pair_vec[i]->servaddr.sin_family = AF_INET;
        socket_items_pair_vec[i]->servaddr.sin_addr.s_addr = INADDR_ANY;
        socket_items_pair_vec[i]->servaddr.sin_port = htons(socket_items_pair_vec[i]->port);
    }
}

socket_client_t::~socket_client_t() {
    for (uint32_t i = 0; i < socket_items_pair_vec.size(); ++i) {
        close(socket_items_pair_vec[i]->socketfd);
    }
};

uint32_t socket_client_t::write_immediate(const uint32_t conn_idx,
                                          const uint8_t* inp,
                                          const uint32_t n) {
    const auto w_n = sendto(socket_items_pair_vec[conn_idx]->socketfd,
                            (const char*)inp,
                            n,
                            MSG_CONFIRM,
                            (const struct sockaddr*)&socket_items_pair_vec[conn_idx]->servaddr,
                            sizeof(socket_items_pair_vec[conn_idx]->servaddr));

    dectnrp_assert(n == w_n, "nof bytes not the same");

    return w_n > 0 ? w_n : 0;
}

uint32_t socket_client_t::write_nto(const uint32_t conn_idx, const uint8_t* inp, const uint32_t n) {
    return socket_items_pair_vec[conn_idx]->items->write_nto(inp, n);
}

uint32_t socket_client_t::write_try(const uint32_t conn_idx, const uint8_t* inp, const uint32_t n) {
    return socket_items_pair_vec[conn_idx]->items->write_try(inp, n);
}

uint32_t socket_client_t::copy_from_items_to_localbuffer(const uint32_t conn_idx) {
    return socket_items_pair_vec[conn_idx]->items->read_nto(buffer_local);
}

}  // namespace dectnrp::application::sockets
