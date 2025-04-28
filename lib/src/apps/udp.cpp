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

#include "dectnrp/apps/udp.hpp"

#include <unistd.h>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::apps {

udp_t::~udp_t() {
    for (size_t i = 0; i < conn_tx.size(); ++i) {
        rm_connection_tx(0);
    }

    for (size_t i = 0; i < conn_rx.size(); ++i) {
        rm_connection_rx(0);
    }
}

std::size_t udp_t::add_connection_tx(const std::string ip, const uint32_t port) {
    connection_t conn;

    if ((conn.socked_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        dectnrp_assert_failure("socked_fd creation failed");
    }

    memset(&conn.sa_in, 0, sizeof(conn.sa_in));

    conn.sa_in.sin_family = AF_INET;
    conn.sa_in.sin_addr.s_addr = inet_addr(ip.c_str());
    conn.sa_in.sin_port = htons(port);

    conn_tx.push_back(conn);

    return conn_tx.size() - 1;
}

std::size_t udp_t::add_connection_rx(const std::string ip,
                                     const uint32_t port,
                                     const std::size_t timeout_us) {
    connection_t conn;

    if ((conn.socked_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        dectnrp_assert_failure("socked_fd creation failed");
    }

    // timeout in case packet round-trip does not work
    if (timeout_us > 0) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = timeout_us;
        if (setsockopt(conn.socked_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            dectnrp_assert_failure("socked_fd timeout setting failed");
        }
    }

    memset(&conn.sa_in, 0, sizeof(conn.sa_in));

    conn.sa_in.sin_family = AF_INET;
    conn.sa_in.sin_addr.s_addr = inet_addr(ip.c_str());
    conn.sa_in.sin_port = htons(port);

    if (bind(conn.socked_fd, (const struct sockaddr*)&conn.sa_in, sizeof(conn.sa_in)) < 0) {
        dectnrp_assert_failure("socked_fd bind failed");
    }

    conn_rx.push_back(conn);

    return conn_rx.size() - 1;
}

void udp_t::rm_connection_tx(const std::size_t idx) {
    dectnrp_assert(idx < conn_tx.size(), "out-of-range");

    close(conn_tx.at(idx).socked_fd);
    conn_tx.erase(conn_tx.begin() + idx);
}

void udp_t::rm_connection_rx(const std::size_t idx) {
    dectnrp_assert(idx < conn_rx.size(), "out-of-range");

    close(conn_rx.at(idx).socked_fd);
    conn_rx.erase(conn_rx.begin() + idx);
}

ssize_t udp_t::tx(const std::size_t idx, const uint8_t* buffer, const std::size_t buffer_len) {
    return sendto(conn_tx.at(idx).socked_fd,
                  buffer,
                  buffer_len,
                  MSG_CONFIRM,
                  (const struct sockaddr*)&conn_tx.at(idx).sa_in,
                  sizeof(conn_tx.at(idx).sa_in));
}

ssize_t udp_t::rx(const std::size_t idx, const uint8_t* buffer, const std::size_t buffer_len) {
    return recvfrom(conn_rx.at(idx).socked_fd, (char*)buffer, buffer_len, MSG_WAITALL, nullptr, 0);
}

}  // namespace dectnrp::apps
