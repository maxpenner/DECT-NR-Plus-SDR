/*
 * Copyright 2023-2024 Maxim Penner, Alexander Poets
 * Copyright 2025-2025 Maxim Penner
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

#include "dectnrp/sections_part4/mac_messages_and_ie/association_release_message.hpp"

#include <utility>

#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::sp4 {

association_release_message_t::association_release_message_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Association_Release_Message;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void association_release_message_t::zero() { release_cause = release_cause_t::not_defined; }

bool association_release_message_t::is_valid() const {
    return common::adt::is_valid(release_cause);
}

uint32_t association_release_message_t::get_packed_size() const { return 1; }

void association_release_message_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that association release message is valid before packing
    dectnrp_assert(is_valid(), "association release message is not valid");
    mac_pdu_offset[0] = std::to_underlying(release_cause) << 4;
}

bool association_release_message_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();
    release_cause = common::adt::from_coded_value<release_cause_t>(mac_pdu_offset[0] >> 4);
    return is_valid();
}

}  // namespace dectnrp::sp4
