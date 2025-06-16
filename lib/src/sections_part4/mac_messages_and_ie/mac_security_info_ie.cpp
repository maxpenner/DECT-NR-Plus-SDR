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

#include "dectnrp/sections_part4/mac_messages_and_ie/mac_security_info_ie.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::sp4 {

mac_security_info_ie_t::mac_security_info_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Security_Info_IE;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void mac_security_info_ie_t::zero() {
    key_index = common::adt::UNDEFINED_NUMERIC_32;
    security_iv_type.mode_1 = security_iv_type_mode_1_t::not_defined;
    hpc = common::adt::UNDEFINED_NUMERIC_32;
}

bool mac_security_info_ie_t::is_valid() const {
    return key_index <= 0b11 && common::adt::is_valid(security_iv_type.mode_1);
}

uint32_t mac_security_info_ie_t::get_packed_size() const { return 5; }

void mac_security_info_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that MAC security info is valid before packing
    dectnrp_assert(is_valid(), "MAC security info is not valid");

    // set required fields in octet 0
    mac_pdu_offset[0] = std::to_underlying(version) << 6;
    mac_pdu_offset[0] |= key_index << 4;
    mac_pdu_offset[0] |= std::to_underlying(security_iv_type.mode_1);

    // set HPC field in octets 1-4
    common::adt::l2b_lower(&mac_pdu_offset[1], hpc, 4);
}

bool mac_security_info_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // check whether security mode is set correctly in version field
    if (mac_pdu_offset[0] >> 6 != std::to_underlying(version_t::mode_1)) {
        return false;
    }

    // unpack remaining fields in octet 0
    key_index = (mac_pdu_offset[0] >> 4) & 0b11;
    security_iv_type.mode_1 =
        common::adt::from_coded_value<security_iv_type_mode_1_t>(mac_pdu_offset[0] & 0xf);

    // unpack HPC field in octets 1-4
    hpc = common::adt::b2l_lower(&mac_pdu_offset[1], 4);

    return is_valid();
}

}  // namespace dectnrp::sp4
