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

#include "dectnrp/application/socket/socketx.hpp"

namespace dectnrp::application::sockets {

socketx_t::socketx_t(const std::vector<uint32_t> ports_,
                     const uint32_t N_item_,
                     const uint32_t N_item_byte_) {
    for (uint32_t i = 0; i < ports_.size(); ++i) {
        socket_items_pair_vec.push_back(
            std::make_unique<socket_items_pair_t>(ports_[i], N_item_, N_item_byte_));
    }
}

}  // namespace dectnrp::application::sockets
