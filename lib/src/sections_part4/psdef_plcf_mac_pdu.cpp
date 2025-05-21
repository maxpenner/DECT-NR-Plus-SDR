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

#include "dectnrp/sections_part4/psdef_plcf_mac_pdu.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

uint32_t psdef_plcf_mac_pdu_t::pack_first_3_header(uint8_t* a_plcf, uint8_t* a_mac_pdu) const {
    dectnrp_assert(a_plcf != nullptr, "destination nullptr");
    dectnrp_assert(a_mac_pdu != nullptr, "destination nullptr");
    dectnrp_assert(plcf_base_effective != nullptr, "source nullptr");

    // pack PLCF
    *plcf_base_effective >> a_plcf;

    // MAC header type
    mac_header_type >> a_mac_pdu;
    a_mac_pdu += mac_header_type.get_packed_size();

    // MAC common header
    *mch_base_effective >> a_mac_pdu;

    return mac_header_type.get_packed_size() + mch_base_effective->get_packed_size();
}

}  // namespace dectnrp::sp4
