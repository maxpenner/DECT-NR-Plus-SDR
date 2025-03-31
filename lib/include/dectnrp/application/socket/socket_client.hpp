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

#include "dectnrp/application/app_client.hpp"
#include "dectnrp/application/socket/socketx.hpp"

namespace dectnrp::application::sockets {

class socket_client_t final : public app_client_t, public socketx_t {
    public:
        explicit socket_client_t(const uint32_t id_,
                                 const common::threads_core_prio_config_t thread_config_,
                                 phy::job_queue_t& job_queue_,
                                 const std::vector<uint32_t> ports_,
                                 const uint32_t N_item_,
                                 const uint32_t N_item_byte_);
        ~socket_client_t();

        socket_client_t() = delete;
        socket_client_t(const socket_client_t&) = delete;
        socket_client_t& operator=(const socket_client_t&) = delete;
        socket_client_t(socket_client_t&&) = delete;
        socket_client_t& operator=(socket_client_t&&) = delete;

        uint32_t get_n_connections() override final { return socket_items_pair_vec.size(); }

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
        [[nodiscard]] uint32_t copy_from_items_to_localbuffer(
            const uint32_t conn_idx) override final;
};

}  // namespace dectnrp::application::sockets
