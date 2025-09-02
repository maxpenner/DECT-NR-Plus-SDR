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

#include "dectnrp/application/socket/socketx.hpp"

namespace dectnrp::application::sockets {

socketx_t::socketx_t(const std::vector<uint32_t> ports_) {
    for (uint32_t i = 0; i < ports_.size(); ++i) {
        udp_vec.push_back(std::make_unique<udp_t>(ports_[i]));
    }
}

}  // namespace dectnrp::application::sockets
