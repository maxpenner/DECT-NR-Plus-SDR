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

#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/power_target_ie.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4::extensions {

power_target_ie_t::power_target_ie_t() {
    mac_multiplexing_header.zero();
    mac_multiplexing_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_multiplexing_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Power_Target_IE;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void power_target_ie_t::zero() { power_target_dBm_coded = common::adt::UNDEFINED_NUMERIC_32; }

bool power_target_ie_t::is_valid() const {
    return 45 <= power_target_dBm_coded && power_target_dBm_coded <= 60;
}

uint32_t power_target_ie_t::get_packed_size() const { return 1; }

void power_target_ie_t::pack(uint8_t* mac_pdu_offset) const {
    dectnrp_assert(is_valid(), "Power Target IE is not valid");
    mac_pdu_offset[0] = static_cast<uint8_t>(power_target_dBm_coded);
}

bool power_target_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();
    power_target_dBm_coded = mac_pdu_offset[0];
    return is_valid();
}

}  // namespace dectnrp::sp4::extensions
