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

#include "dectnrp/phy/harq/process.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

namespace dectnrp::phy::harq {

#define ASSERT_FULLY_LOCKED dectnrp_assert(is_outer_locked_inner_locked(), "incorrect lock state")

uint32_t process_t::get_id() const {
    ASSERT_FULLY_LOCKED;
    return id;
};
uint32_t process_t::get_PLCF_type() const {
    ASSERT_FULLY_LOCKED;
    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "unknown PLCF type");
    return PLCF_type;
};

uint32_t process_t::get_network_id() const {
    ASSERT_FULLY_LOCKED;
    dectnrp_assert(network_id > 0, "unknown network_id");
    return network_id;
}

const sp3::packet_sizes_t& process_t::get_packet_sizes() const {
    ASSERT_FULLY_LOCKED;
    return packet_sizes;
};

uint32_t process_t::get_rv() const {
    ASSERT_FULLY_LOCKED;
    dectnrp_assert(rv <= constants::rv_max, "unknown rv");
    return rv;
}

void process_t::reset() {
    ASSERT_FULLY_LOCKED;
    PLCF_type = 0;
    network_id = 0;
    packet_sizes = sp3::packet_sizes_t();
    rv = 0;
}

}  // namespace dectnrp::phy::harq
