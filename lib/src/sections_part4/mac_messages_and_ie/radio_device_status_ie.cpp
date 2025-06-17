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

#include "dectnrp/sections_part4/mac_messages_and_ie/radio_device_status_ie.hpp"

#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::sp4 {

radio_device_status_ie_t::radio_device_status_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::Length_Field_1_Bit;
    mac_mux_header.length = 1;
    mac_mux_header.ie_type.mac_ext_11_len_1 =
        mac_multiplexing_header_t::ie_type_mac_ext_11_len_1_t::Radio_Device_Status_IE;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void radio_device_status_ie_t::zero() {
    status_flag = status_flag_t::not_defined;
    duration = duration_t::not_defined;
}

bool radio_device_status_ie_t::is_valid() const {
    return common::adt::is_valid(status_flag) && common::adt::is_valid(duration);
}

uint32_t radio_device_status_ie_t::get_packed_size() const { return 1; }

void radio_device_status_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that Radio Device Status IE is valid before packing
    dectnrp_assert(is_valid(), "Radio Device Status IE is not valid");

    mac_pdu_offset[0] = std::to_underlying(status_flag) << 4;
    mac_pdu_offset[0] |= std::to_underlying(duration);
}

bool radio_device_status_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    status_flag = common::adt::from_coded_value<status_flag_t>((mac_pdu_offset[0] >> 4) & 0b11);
    duration = common::adt::from_coded_value<duration_t>(mac_pdu_offset[0] & 0xf);

    return is_valid();
}

}  // namespace dectnrp::sp4
