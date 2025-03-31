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

#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/forward_to_ie.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"

namespace dectnrp::section4::extensions {

forward_to_ie_t::forward_to_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.length = 1;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Forward_To_IE;

    zero();

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void forward_to_ie_t::zero() {
    source_address = mac_architecture::identity_t::LongRadioDeviceID_broadcast;
    sink_address = mac_architecture::identity_t::LongRadioDeviceID_broadcast;
}

bool forward_to_ie_t::is_valid() const {
    return (source_address != mac_architecture::identity_t::LongRadioDeviceID_broadcast) &&
           (sink_address != mac_architecture::identity_t::LongRadioDeviceID_broadcast);
}

uint32_t forward_to_ie_t::get_packed_size() const { return 8; }

void forward_to_ie_t::pack(uint8_t* mac_pdu_offset) const {
    dectnrp_assert(is_valid(), "Forward To IE is not valid");

    common::adt::l2b_lower(&mac_pdu_offset[0], source_address, 4);
    common::adt::l2b_lower(&mac_pdu_offset[4], sink_address, 4);
}

bool forward_to_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    source_address = common::adt::b2l_lower(&mac_pdu_offset[0], 4);
    sink_address = common::adt::b2l_lower(&mac_pdu_offset[4], 4);
    return is_valid();
}

}  // namespace dectnrp::section4::extensions
