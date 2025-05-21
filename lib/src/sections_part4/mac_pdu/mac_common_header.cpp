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

#include "dectnrp/sections_part4/mac_pdu/mac_common_header.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"

namespace dectnrp::sp4 {

void data_mac_pdu_header_t::zero() {
    Reserved = 0;
    Reset = 0;
    Sequence_number = 0;
}

bool data_mac_pdu_header_t::is_valid() const {
    if (Reserved != 0) {
        return false;
    }

    if (1 < Reset) {
        return false;
    }

    if (common::adt::bitmask_lsb<12>() < Sequence_number) {
        return false;
    }

    return true;
}

void data_mac_pdu_header_t::pack(uint8_t* mch_front) const {
    dectnrp_assert(is_valid(), "invalid");

    mch_front[0] = Reserved << 5;
    mch_front[0] |= Reset << 4;
    mch_front[0] |= Sequence_number >> 8;
    mch_front[1] = Sequence_number & common::adt::bitmask_lsb<8>();
}

bool data_mac_pdu_header_t::unpack(const uint8_t* mch_front) {
    Reserved = (mch_front[0] >> 5) & 0b111;
    Reset = (mch_front[0] >> 4) & 0b1;
    Sequence_number = ((uint32_t)(mch_front[0] & 0b1111) << 8) + (uint32_t)mch_front[1];

    return is_valid();
}

void beacon_header_t::zero() {
    Network_ID_3_lsb = 0;
    Transmitter_Address = 0;
}

bool beacon_header_t::is_valid() const {
    if (common::adt::bitmask_lsb<24>() < Network_ID_3_lsb) {
        return false;
    }

    if (!mac_architecture::identity_t::is_valid_LongRadioDeviceID(Transmitter_Address)) {
        return false;
    }

    return true;
}

void beacon_header_t::pack(uint8_t* mch_front) const {
    dectnrp_assert(is_valid(), "invalid");

    common::adt::l2b_upper(&mch_front[0], Network_ID_3_lsb, 3);
    common::adt::l2b_lower(&mch_front[3], Transmitter_Address, 4);
}

bool beacon_header_t::unpack(const uint8_t* mch_front) {
    Network_ID_3_lsb = common::adt::b2l_upper(&mch_front[0], 3);
    Transmitter_Address = common::adt::b2l_lower(&mch_front[3], 4);

    return is_valid();
}

void beacon_header_t::set_Network_ID_3_lsb(const uint32_t Network_ID) {
    Network_ID_3_lsb = Network_ID & common::adt::bitmask_lsb<24>();
}

void unicast_header_t::zero() {
    Reserved = 0;
    Reset = 0;
    Sequence_number = 0;
    Receiver_Address = 0;
    Transmitter_Address = 0;
}

bool unicast_header_t::is_valid() const {
    if (Reserved != 0) {
        return false;
    }

    if (1 < Reset) {
        return false;
    }

    if (common::adt::bitmask_lsb<12>() < Sequence_number) {
        return false;
    }

    if (!mac_architecture::identity_t::is_valid_LongRadioDeviceID(Transmitter_Address)) {
        return false;
    }

    if (!mac_architecture::identity_t::is_valid_LongRadioDeviceID(Receiver_Address)) {
        return false;
    }

    return true;
}

void unicast_header_t::pack(uint8_t* mch_front) const {
    dectnrp_assert(is_valid(), "invalid");

    mch_front[0] = Reserved << 5;
    mch_front[0] |= Reset << 4;
    mch_front[0] |= Sequence_number >> 8;
    mch_front[1] = Sequence_number & common::adt::bitmask_lsb<8>();
    common::adt::l2b_lower(&mch_front[2], Receiver_Address, 4);
    common::adt::l2b_lower(&mch_front[6], Transmitter_Address, 4);
}

bool unicast_header_t::unpack(const uint8_t* mch_front) {
    Reserved = (mch_front[0] >> 5) & 0b111;
    Reset = (mch_front[0] >> 4) & 0b1;
    Sequence_number = ((uint32_t)(mch_front[0] & 0b1111) << 8) + (uint32_t)mch_front[1];
    Receiver_Address = common::adt::b2l_lower(&mch_front[2], 4);
    Transmitter_Address = common::adt::b2l_lower(&mch_front[6], 4);

    return is_valid();
}

void rd_broadcasting_header_t::zero() {
    Reserved = 0;
    Reset = 0;
    Sequence_number = 0;
    Transmitter_Address = 0;
}

bool rd_broadcasting_header_t::is_valid() const {
    if (Reserved != 0) {
        return false;
    }

    if (1 < Reset) {
        return false;
    }

    if (common::adt::bitmask_lsb<12>() < Sequence_number) {
        return false;
    }

    if (!mac_architecture::identity_t::is_valid_LongRadioDeviceID(Transmitter_Address)) {
        return false;
    }

    return true;
}

void rd_broadcasting_header_t::pack(uint8_t* mch_front) const {
    dectnrp_assert(is_valid(), "invalid");

    mch_front[0] = (Reserved << 5);
    mch_front[0] |= Reset << 4;
    mch_front[0] |= Sequence_number >> 8;
    mch_front[1] = Sequence_number & common::adt::bitmask_lsb<8>();
    common::adt::l2b_lower(&mch_front[2], Transmitter_Address, 4);
}

bool rd_broadcasting_header_t::unpack(const uint8_t* mch_front) {
    Reserved = (mch_front[0] >> 5) & 0b111;
    Reset = (mch_front[0] >> 4) & 0b1;
    Sequence_number = ((uint32_t)(mch_front[0] & 0b1111) << 8) + (uint32_t)mch_front[1];
    Transmitter_Address = common::adt::b2l_lower(&mch_front[2], 4);

    return is_valid();
}

}  // namespace dectnrp::sp4
