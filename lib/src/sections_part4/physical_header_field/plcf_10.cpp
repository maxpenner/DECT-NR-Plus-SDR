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

#include "dectnrp/sections_part4/physical_header_field/plcf_10.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"

namespace dectnrp::section4 {

void plcf_10_t::zero() {
    plcf_base_t::zero();

    ShortNetworkID = 0;
    TransmitterIdentity = 0;
    TransmitPower = 0;
    Reserved = 0;
    DFMCS = 0;
}

bool plcf_10_t::is_valid() const {
    if (HeaderFormat != 0) {
        return false;
    }

    if (!mac_architecture::identity_t::is_valid_ShortNetworkID(ShortNetworkID)) {
        return false;
    }

    if (!mac_architecture::identity_t::is_valid_ShortRadioDeviceID(TransmitterIdentity)) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < TransmitPower) {
        return false;
    }

    if (Reserved != 0) {
        return false;
    }

    // has only three bits
    if (common::adt::bitmask_lsb<3>() < DFMCS) {
        return false;
    }

    return true;
}

void plcf_10_t::pack(uint8_t* plcf_front) const {
    plcf_base_t::pack(plcf_front);

    dectnrp_assert(is_valid(), "invalid");

    plcf_front[1] = ShortNetworkID;
    common::adt::l2b_lower(&plcf_front[2], TransmitterIdentity, 2);
    plcf_front[4] = TransmitPower << 4;
    plcf_front[4] |= Reserved << 3;
    plcf_front[4] |= DFMCS;
}

bool plcf_10_t::unpack(const uint8_t* plcf_front) {
    if (!plcf_base_t::unpack(plcf_front)) {
        return false;
    }

    ShortNetworkID = plcf_front[1];
    TransmitterIdentity = common::adt::b2l_lower(&plcf_front[2], 2);
    TransmitPower = (plcf_front[4] >> 4) & 0b1111;
    Reserved = (plcf_front[4] >> 3) & 0b1;
    DFMCS = plcf_front[4] & 0b111;

    return is_valid();
}

}  // namespace dectnrp::section4
