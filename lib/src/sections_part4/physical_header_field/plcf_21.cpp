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

#include "dectnrp/sections_part4/physical_header_field/plcf_21.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"

namespace dectnrp::sp4 {

void plcf_21_t::zero() {
    plcf_base_t::zero();

    ShortNetworkID = 0;
    TransmitterIdentity = 0;
    TransmitPower = 0;
    DFMCS = 0;
    ReceiverIdentity = 0;
    NumberOfSpatialStreams = 0;
    Reserved = 0;
    FeedbackFormat = 0;
}

bool plcf_21_t::is_valid() const {
    if (HeaderFormat != 1) {
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

    // 1024-QAM R=5/6
    if (11 < DFMCS) {
        return false;
    }

    if (!mac_architecture::identity_t::is_valid_ShortRadioDeviceID(ReceiverIdentity)) {
        return false;
    }

    if (common::adt::bitmask_lsb<2>() < NumberOfSpatialStreams) {
        return false;
    }

    if (Reserved != 0) {
        return false;
    }

    if (common::adt::bitmask_lsb<4>() < FeedbackFormat) {
        return false;
    }

    return true;
}

void plcf_21_t::pack(uint8_t* plcf_front) const {
    plcf_base_t::pack(plcf_front);

    dectnrp_assert(is_valid(), "invalid");

    plcf_front[1] = ShortNetworkID;
    common::adt::l2b_lower(&plcf_front[2], TransmitterIdentity, 2);
    plcf_front[4] = TransmitPower << 4;
    plcf_front[4] |= DFMCS;
    common::adt::l2b_lower(&plcf_front[5], ReceiverIdentity, 2);
    plcf_front[7] = NumberOfSpatialStreams << 6;
    plcf_front[7] |= Reserved;
    plcf_front[8] = FeedbackFormat << 4;

    feedback_info_pool.pack(FeedbackFormat, &plcf_front[8]);
}

bool plcf_21_t::unpack(const uint8_t* plcf_front) {
    if (!plcf_base_t::unpack(plcf_front)) {
        return false;
    }

    ShortNetworkID = plcf_front[1];
    TransmitterIdentity = common::adt::b2l_lower(&plcf_front[2], 2);
    TransmitPower = (plcf_front[4] >> 4) & 0b1111;
    DFMCS = plcf_front[4] & 0b1111;
    ReceiverIdentity = common::adt::b2l_lower(&plcf_front[5], 2);
    NumberOfSpatialStreams = (plcf_front[7] >> 6) & 0b11;
    Reserved = plcf_front[7] & 0b111111;
    FeedbackFormat = (plcf_front[8] >> 4) & 0b1111;

    if (!feedback_info_pool.unpack(FeedbackFormat, &plcf_front[8])) {
        return false;
    }

    return is_valid();
}

void plcf_21_t::set_NumberOfSpatialStreams(const int32_t N_SS) {
    NumberOfSpatialStreams = N_SS_coded_lut.at(N_SS);
}

uint32_t plcf_21_t::get_N_SS() const { return N_SS_coded_lut_rev.at(NumberOfSpatialStreams); }

uint32_t plcf_21_t::get_DFRedundancyVersion() const { return 0; }

}  // namespace dectnrp::sp4
