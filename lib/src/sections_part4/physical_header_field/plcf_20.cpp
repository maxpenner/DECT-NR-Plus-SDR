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

#include "dectnrp/sections_part4/physical_header_field/plcf_20.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section4 {

void plcf_20_t::zero() {
    plcf_base_t::zero();

    ShortNetworkID = 0;
    TransmitterIdentity = 0;
    ReceiverIdentity = 0;
    NumberOfSpatialStreams = 0;
    DFRedundancyVersion = 0;
    DFNewDataIndication = 0;
    DFHARQProcessNumber = 0;
    FeedbackFormat = 0;
    FeedbackInfo = 0;
}

void plcf_20_t::pack(uint8_t* plcf_front) const {
    plcf_base_t::pack(plcf_front);

    plcf_front[1] = ShortNetworkID;
    common::adt::l2b_lower(&plcf_front[2], TransmitterIdentity, 2);
    plcf_front[4] = TransmitPower << 4;
    plcf_front[4] |= DFMCS;
    common::adt::l2b_lower(&plcf_front[5], ReceiverIdentity, 2);
    plcf_front[7] = NumberOfSpatialStreams << 6;
    plcf_front[7] |= DFRedundancyVersion << 4;
    plcf_front[7] |= DFNewDataIndication << 3;
    plcf_front[7] |= DFHARQProcessNumber;
    plcf_front[8] = FeedbackFormat << 4;

    feedback_info_pool.pack(FeedbackFormat, FeedbackInfo);

    plcf_front[8] |= FeedbackInfo >> 8;
    plcf_front[9] = FeedbackInfo & common::adt::bitmask<uint8_t, 8, 0>();
}

bool plcf_20_t::unpack(const uint8_t* plcf_front) {
    if (!plcf_base_t::unpack(plcf_front)) {
        return false;
    }

    // dectnrp_assert(HeaderFormat == 0, "header format should be 0");

    ShortNetworkID = plcf_front[1];
    TransmitterIdentity = common::adt::b2l_lower(&plcf_front[2], 2);
    TransmitPower = (plcf_front[4] >> 4) & 0b1111;
    DFMCS = plcf_front[4] & 0b1111;
    ReceiverIdentity = common::adt::b2l_lower(&plcf_front[5], 2);
    NumberOfSpatialStreams = (plcf_front[7] >> 6) & 0b11;
    DFRedundancyVersion = (plcf_front[7] >> 4) & 0b11;
    DFNewDataIndication = (plcf_front[7] >> 3) & 0b1;
    DFHARQProcessNumber = plcf_front[7] & 0b111;
    FeedbackFormat = (plcf_front[8] >> 4) & 0b1111;
    FeedbackInfo = ((uint32_t)(plcf_front[8] & 0b1111) << 8) + (((uint32_t)plcf_front[9]));

    if (!feedback_info_pool.unpack(FeedbackFormat, FeedbackInfo)) {
        return false;
    }

    return true;
}

void plcf_20_t::set_NumberOfSpatialStreams(const int32_t N_SS) {
    NumberOfSpatialStreams = N_SS_coded_lut.at(N_SS);
}

uint32_t plcf_20_t::get_N_SS() const { return N_SS_coded_lut_rev.at(NumberOfSpatialStreams); }

uint32_t plcf_20_t::get_DFRedundancyVersion() const { return DFRedundancyVersion; }

}  // namespace dectnrp::section4
