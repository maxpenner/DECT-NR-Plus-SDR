/*
 * Copyright 2023-present Maxim Penner
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

#include <cstdint>
#include <memory>
#include <vector>

#include "dectnrp/application/socket/udp.hpp"

namespace dectnrp::application::sockets {

class socketx_t {
    public:
        explicit socketx_t(const std::vector<uint32_t> ports_);
        virtual ~socketx_t() = default;

        socketx_t() = delete;
        socketx_t(const socketx_t&) = delete;
        socketx_t& operator=(const socketx_t&) = delete;
        socketx_t(socketx_t&&) = delete;
        socketx_t& operator=(socketx_t&&) = delete;

    protected:
        std::vector<std::unique_ptr<udp_t>> udp_vec;
};

}  // namespace dectnrp::application::sockets
