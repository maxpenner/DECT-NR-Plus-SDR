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

#include "dectnrp/sections_part4/physical_header_field/plcf_base.hpp"

#include <algorithm>
#include <cmath>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

void plcf_base_t::zero() {
    HeaderFormat = 0;
    PacketLengthType = 0;
    PacketLength_m1 = 0;
}

bool plcf_base_t::is_valid() const {
    if (1 < HeaderFormat) {
        return false;
    }

    if (1 < PacketLengthType) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < PacketLength_m1) {
        return false;
    }

    return true;
}

void plcf_base_t::pack(uint8_t* plcf_front) const {
    dectnrp_assert(plcf_base_t::is_valid(), "invalid");

    plcf_front[0] = HeaderFormat << 5;
    plcf_front[0] |= PacketLengthType << 4;
    plcf_front[0] |= PacketLength_m1;
}

bool plcf_base_t::unpack(const uint8_t* plcf_front) {
    HeaderFormat = plcf_front[0] >> 5;
    PacketLengthType = (plcf_front[0] >> 4) & 0b1;
    PacketLength_m1 = plcf_front[0] & 0b1111;

    return plcf_base_t::is_valid();
}

void plcf_base_t::set_PacketLength_m1(const int32_t PacketLength) {
    PacketLength_m1 = PacketLength - 1;
}

uint32_t plcf_base_t::get_PacketLength() const { return PacketLength_m1 + 1; }

void plcf_base_t::set_TransmitPower(const int32_t TransmitPower_dBm) {
    const auto lower =
        std::lower_bound(tx_power_table.begin(), tx_power_table.end(), TransmitPower_dBm);

    TransmitPower =
        lower == tx_power_table.end() ? tx_power_table.size() - 1 : lower - tx_power_table.begin();
}

}  // namespace dectnrp::sp4
