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

#include "dectnrp/application/app_server.hpp"
#include "dectnrp/application/socket/socketx.hpp"

namespace dectnrp::application::sockets {

class socket_server_t final : public app_server_t, public socketx_t {
    public:
        explicit socket_server_t(const uint32_t id_,
                                 const common::threads_core_prio_config_t thread_config_,
                                 phy::job_queue_t& job_queue_,
                                 const std::vector<uint32_t> ports_,
                                 const uint32_t N_item_,
                                 const uint32_t N_item_byte_);
        ~socket_server_t();

        socket_server_t() = delete;
        socket_server_t(const socket_server_t&) = delete;
        socket_server_t& operator=(const socket_server_t&) = delete;
        socket_server_t(socket_server_t&&) = delete;
        socket_server_t& operator=(socket_server_t&&) = delete;

        void work_sc() override final;
        uint32_t get_n_connections() override final { return socket_items_pair_vec.size(); }

        items_level_report_t get_items_level_report_nto(const uint32_t conn_idx,
                                                        const uint32_t n) const override final;
        items_level_report_t get_items_level_report_try(const uint32_t conn_idx,
                                                        const uint32_t n) const override final;

        uint32_t read_nto(const uint32_t conn_idx, uint8_t* dst) override final;

        uint32_t read_try(const uint32_t conn_idx, uint8_t* dst) override final;
};

}  // namespace dectnrp::application::sockets
