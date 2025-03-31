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

#include <memory>

#include "dectnrp/application/items.hpp"

namespace dectnrp::application::sockets {

class socket_items_pair_t {
    public:
        explicit socket_items_pair_t(const uint32_t port_,
                                     const uint32_t N_item_,
                                     const uint32_t N_item_byte_)
            : port(port_),
              items(std::make_unique<items_t>(N_item_, N_item_byte_)) {};
        ~socket_items_pair_t() = default;

        socket_items_pair_t() = delete;
        socket_items_pair_t(const socket_items_pair_t&) = delete;
        socket_items_pair_t& operator=(const socket_items_pair_t&) = delete;
        socket_items_pair_t(socket_items_pair_t&&) = delete;
        socket_items_pair_t& operator=(socket_items_pair_t&&) = delete;

        friend class socket_server_t;
        friend class socket_client_t;

    private:
        const uint32_t port;
        int socketfd;
        struct sockaddr_in servaddr;
        std::unique_ptr<items_t> items;
};

}  // namespace dectnrp::application::sockets
