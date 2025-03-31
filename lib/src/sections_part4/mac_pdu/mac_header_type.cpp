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

#include "dectnrp/sections_part4/mac_pdu/mac_header_type.hpp"

#include <utility>

#include "dectnrp/common/adt/miscellaneous.hpp"

namespace dectnrp::section4 {

void mac_header_type_t::zero() {
    Version = version_ec::not_defined;
    MAC_security = mac_security_ec::not_defined;
    MAC_header_type = mac_header_type_ec::not_defined;
}

void mac_header_type_t::pack(uint8_t* mac_pdu_front) const {
    mac_pdu_front[0] = std::to_underlying(Version) << 6;
    mac_pdu_front[0] |= std::to_underlying(MAC_security) << 4;
    mac_pdu_front[0] |= std::to_underlying(MAC_header_type);
}

bool mac_header_type_t::unpack(const uint8_t* mac_pdu_front) {
    uint32_t val = 0;

    val = (mac_pdu_front[0] >> 6) & 0b11;
    if ((Version = get_version_ec(val)) == version_ec::not_defined) {
        return false;
    }

    val = (mac_pdu_front[0] >> 4) & 0b11;
    if ((MAC_security = get_mac_security_ec(val)) == mac_security_ec::not_defined) {
        return false;
    }

    val = mac_pdu_front[0] & 0b1111;
    if ((MAC_header_type = get_mac_header_type_ec(val)) == mac_header_type_ec::not_defined) {
        return false;
    }

    return true;
}

}  // namespace dectnrp::section4
