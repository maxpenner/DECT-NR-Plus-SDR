/*
 * Copyright 2023-2025 Maxim Penner, Mattes Wassmann
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

#include "dectnrp/sections_part4/physical_header_field/extension/plcf_22.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::section4::extension {

void plcf_22_t::zero() {
    plcf_base_t::zero();
    // set every field to zero
    TransmitterIdentity = 0;
    dl = true;
    forward_despite_crc_error = true;
    next_scheduled_packet_stf = next_scheduled_packet_stf_t::not_defined;
    next_scheduled_packet_has_plcf = true;
    NumberOfSpatialStreams = 0;
    FeedbackFormat = feedback_info_t::No_feedback;
    FeedbackInfo = 0;
}

void plcf_22_t::pack(uint8_t* plcf_front) const {
    plcf_base_t::pack(plcf_front);

    common::adt::l2b_lower(&plcf_front[1], TransmitterIdentity, 4);
    plcf_front[5] = TransmitPower << 4;
    plcf_front[5] |= DFMCS;
    plcf_front[6] = dl << 7;
    plcf_front[6] |= forward_despite_crc_error << 6;
    plcf_front[6] |= std::to_underlying(next_scheduled_packet_stf) << 4;
    plcf_front[6] |= next_scheduled_packet_has_plcf << 3;
    plcf_front[6] |= NumberOfSpatialStreams;
    plcf_front[7] = 0;
    plcf_front[8] = FeedbackFormat << 4;

    feedback_info_pool.pack(FeedbackFormat, FeedbackInfo);

    plcf_front[8] |= FeedbackInfo >> 8;
    plcf_front[9] = FeedbackInfo & common::adt::bitmask_lsb<8>();
}

bool plcf_22_t::unpack(const uint8_t* plcf_front) {
    // unpack first octet
    if (!plcf_base_t::unpack(plcf_front)) {
        return false;
    }

    TransmitterIdentity = common::adt::b2l_lower(&plcf_front[1], 4);
    TransmitPower = plcf_front[5] >> 4;
    DFMCS = plcf_front[5] & common::adt::bitmask_lsb<4>();
    dl = plcf_front[6] >> 7;
    forward_despite_crc_error = (plcf_front[6] >> 6) & 1;
    next_scheduled_packet_stf = common::adt::from_coded_value<next_scheduled_packet_stf_t>(
        (plcf_front[6] >> 4) & common::adt::bitmask_lsb<2>());
    next_scheduled_packet_has_plcf = (plcf_front[6] >> 3) & 1;
    NumberOfSpatialStreams = plcf_front[6] & common::adt::bitmask_lsb<2>();
    FeedbackFormat = plcf_front[8] >> 4;
    FeedbackInfo = (plcf_front[8] & common::adt::bitmask_lsb<4>()) << 8;
    FeedbackInfo |= plcf_front[9];

    if (!feedback_info_pool.unpack(FeedbackFormat, FeedbackInfo)) {
        return false;
    }

    return true;
}

void plcf_22_t::set_NumberOfSpatialStreams(const uint32_t N_SS) {
    NumberOfSpatialStreams = N_SS_coded_lut.at(N_SS);
}

uint32_t plcf_22_t::get_N_SS() const { return N_SS_coded_lut_rev.at(NumberOfSpatialStreams); }

uint32_t plcf_22_t::get_DFRedundancyVersion() const { return 0; }

}  // namespace dectnrp::section4::extension
