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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <vector>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/common/thread/watch.hpp"

namespace dectnrp::common::adt {

template <typename T>
    requires(std::is_trivially_copyable_v<T>)
class tcp_scope_t {
    public:
        explicit tcp_scope_t(const uint32_t port_first, const uint32_t nof_antennas_max)
            : TCP_packet_size_max_T(TCP_packet_size_max_byte / sizeof(T)) {
            // preallocate
            connection_vec.resize(nof_antennas_max);

            // assume the best
            initialized = true;

            for (uint32_t i = 0; i < nof_antennas_max; ++i) {
                if ((connection_vec[i].sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    dectnrp_log_wrn("Unable to create socket.");

                    initialized = false;
                    break;
                }

                connection_vec[i].serv_addr.sin_family = AF_INET;
                connection_vec[i].serv_addr.sin_port = htons(port_first + i);

                if (inet_pton(AF_INET, "127.0.0.1", &connection_vec[i].serv_addr.sin_addr) <= 0) {
                    dectnrp_log_wrn("Unable to convert address.");

                    initialized = false;
                    break;
                }

                if ((connection_vec[i].client_fd =
                         connect(connection_vec[i].sock,
                                 (struct sockaddr*)&connection_vec[i].serv_addr,
                                 sizeof(connection_vec[i].serv_addr))) < 0) {
                    dectnrp_log_wrn(
                        "Unable to connect. GNU Radio .grc started with correct ports starting at {}? Called from multiple threads?",
                        port_first);

                    initialized = false;
                    break;
                }

                // GNU Radio requires some time to start next TCP server
                common::watch_t::sleep<dectnrp::common::milli>(100);
            }

            if (initialized) {
                dectnrp_log_wrn("tcp_scope_t initialized for port_first={}", port_first);
            } else {
                cleanup();
                dectnrp_log_wrn("tcp_scope_t NOT initialized for port_first={}", port_first);
            }
        };

        ~tcp_scope_t() { cleanup(); }

        tcp_scope_t() = delete;
        tcp_scope_t(const tcp_scope_t&) = delete;
        tcp_scope_t& operator=(const tcp_scope_t&) = delete;
        tcp_scope_t(tcp_scope_t&&) = delete;
        tcp_scope_t& operator=(tcp_scope_t&&) = delete;

        void send_to_scope(const std::vector<T*> in, const uint32_t length) {
            if (!initialized) {
                return;
            }

            dectnrp_assert(in.size() == connection_vec.size(), "Input size not scope size.");

            uint32_t length_cnt = 0;

            while (length_cnt < length) {
                // sendable
                const uint32_t residual_T = length - length_cnt;
                const uint32_t nof_tcp_scope_t = std::min(TCP_packet_size_max_T, residual_T);
                const uint32_t nof_tx_bytes = nof_tcp_scope_t * sizeof(T);

                // send on every connection
                for (uint32_t i = 0; i < in.size(); ++i) {
                    if (nof_tx_bytes != send(connection_vec[i].sock,
                                             (const char*)&in[i][length_cnt],
                                             nof_tx_bytes,
                                             0)) {
                        dectnrp_assert_failure("Send returned incorrect amount of bytes sent.");
                    }
                }

                length_cnt += nof_tcp_scope_t;
            }
        }

        void cleanup() {
            for (auto& elem : connection_vec) {
                if (elem.client_fd >= 0) {
                    close(elem.client_fd);
                }

                elem.client_fd = -1;
            }

            for (auto& elem : connection_vec) {
                if (elem.sock >= 0) {
                    close(elem.sock);
                }

                elem.sock = -1;
            }
        }

    private:
        static constexpr int TCP_packet_size_max_byte = 1472;

        const uint32_t TCP_packet_size_max_T;

        bool initialized;

        struct connection_t {
                int sock{-1};
                int client_fd{-1};
                struct sockaddr_in serv_addr;
        };

        std::vector<connection_t> connection_vec;
};

}  // namespace dectnrp::common::adt
