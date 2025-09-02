/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/sections_part4/mac_architecture/identity.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4::mac_architecture {

identity_t::identity_t(uint32_t NetworkID_,
                       const uint32_t LongRadioDeviceID_,
                       const uint32_t ShortRadioDeviceID_)
    : NetworkID(NetworkID_),
      ShortNetworkID(full_to_short_network_id(NetworkID)),
      LongRadioDeviceID(LongRadioDeviceID_),
      ShortRadioDeviceID(ShortRadioDeviceID_) {
    dectnrp_assert(NetworkID != NetworkID_reserved, "Network ID incorrect.");

    dectnrp_assert(LongRadioDeviceID != LongRadioDeviceID_reserved &&
                       LongRadioDeviceID != LongRadioDeviceID_backend &&
                       LongRadioDeviceID != LongRadioDeviceID_broadcast,
                   "Long Radio Device ID incorrect.");

    dectnrp_assert(ShortRadioDeviceID != ShortRadioDeviceID_reserved &&
                       ShortRadioDeviceID != ShortRadioDeviceID_broadcast,
                   "Short Radio Device ID incorrect.");
}

bool identity_t::is_valid_NetworkID(const uint32_t NetworkID) {
    if (NetworkID == mac_architecture::identity_t::NetworkID_reserved) {
        return false;
    }

    return true;
}

bool identity_t::is_valid_ShortNetworkID(const uint32_t ShortNetworkID) {
    if (ShortNetworkID == mac_architecture::identity_t::NetworkID_reserved) {
        return false;
    }

    if (common::adt::bitmask_lsb<8>() < ShortNetworkID) {
        return false;
    }

    return true;
}

bool identity_t::is_valid_LongRadioDeviceID(const uint32_t LongRadioDeviceID) {
    if (LongRadioDeviceID == mac_architecture::identity_t::LongRadioDeviceID_reserved) {
        return false;
    }

    return true;
}

bool identity_t::is_valid_ShortRadioDeviceID(const uint32_t ShortRadioDeviceID) {
    if (ShortRadioDeviceID == mac_architecture::identity_t::ShortRadioDeviceID_reserved) {
        return false;
    }

    if (common::adt::bitmask_lsb<16>() < ShortRadioDeviceID) {
        return false;
    }

    return true;
}

}  // namespace dectnrp::sp4::mac_architecture
