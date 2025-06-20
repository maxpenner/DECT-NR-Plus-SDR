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

#include "dectnrp/application/application_client.hpp"
#include "dectnrp/application/socket/socketx.hpp"

namespace dectnrp::application::sockets {

class socket_client_t final : public application_client_t, public socketx_t {
    public:
        explicit socket_client_t(const uint32_t id_,
                                 const common::threads_core_prio_config_t thread_config_,
                                 phy::job_queue_t& job_queue_,
                                 const std::vector<uint32_t> ports,
                                 const queue_size_t queue_size);
        ~socket_client_t();

        socket_client_t() = delete;
        socket_client_t(const socket_client_t&) = delete;
        socket_client_t& operator=(const socket_client_t&) = delete;
        socket_client_t(socket_client_t&&) = delete;
        socket_client_t& operator=(socket_client_t&&) = delete;

        uint32_t get_n_connections() const override final { return udp_vec.size(); }

        uint32_t write_immediate(const uint32_t conn_idx,
                                 const uint8_t* inp,
                                 const uint32_t n) override final;
        [[nodiscard]] uint32_t write_nto(const uint32_t conn_idx,
                                         const uint8_t* inp,
                                         const uint32_t n) override final;
        [[nodiscard]] uint32_t write_try(const uint32_t conn_idx,
                                         const uint8_t* inp,
                                         const uint32_t n) override final;

    private:
        bool filter_egress_datagram(const uint32_t conn_idx) override final;

        [[nodiscard]] uint32_t copy_from_queue_to_localbuffer(
            const uint32_t conn_idx) override final;
};

}  // namespace dectnrp::application::sockets
