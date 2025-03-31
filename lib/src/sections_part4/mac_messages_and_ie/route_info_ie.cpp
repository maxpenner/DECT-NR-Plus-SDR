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

#include "dectnrp/sections_part4/mac_messages_and_ie/route_info_ie.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"

namespace dectnrp::section4 {

route_info_ie_t::route_info_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Route_Info_IE;

    zero();

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void route_info_ie_t::zero() {
    sink_address = common::adt::UNDEFINED_NUMERIC_32;
    route_cost = common::adt::UNDEFINED_NUMERIC_32;
    application_sequence_number = common::adt::UNDEFINED_NUMERIC_32;
}

bool route_info_ie_t::is_valid() const {
    return route_cost <= 0xff && application_sequence_number <= 0xff;
}

uint32_t route_info_ie_t::get_packed_size() const { return 6; }

void route_info_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that route info IE is valid before packing
    dectnrp_assert(is_valid(), "route info IE is not valid");

    // set sink address field in octets 0-3
    common::adt::l2b_lower(&mac_pdu_offset[0], sink_address, 4);

    // set route cost field in octet 4
    mac_pdu_offset[4] = route_cost;

    // set application sequence number field in octet 5
    mac_pdu_offset[5] = application_sequence_number;
}

bool route_info_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    sink_address = common::adt::b2l_lower(&mac_pdu_offset[0], 4);
    route_cost = mac_pdu_offset[4];
    application_sequence_number = mac_pdu_offset[5];

    return true;
}

}  // namespace dectnrp::section4
