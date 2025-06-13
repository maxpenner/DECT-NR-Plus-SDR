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

#include "dectnrp/phy/interfaces/layers_downwards/mac_lower.hpp"
#include "dectnrp/upper/p2p/data/rd.hpp"
#include "dectnrp/upper/tpoint_config.hpp"
#include "dectnrp/upper/tpoint_state.hpp"

namespace dectnrp::upper::tfw::p2p {

struct args_t {
        const tpoint_config_t& tpoint_config;
        phy::mac_lower_t& mac_lower;
        tpoint_state_t::leave_callback_t& leave_callback;
        rd_t& rd;
};

}  // namespace dectnrp::upper::tfw::p2p
