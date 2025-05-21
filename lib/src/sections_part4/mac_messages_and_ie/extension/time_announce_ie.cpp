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

#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/time_announce_ie.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4::extensions {

time_announce_ie_t::time_announce_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Time_Announce_IE;

    zero();

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void time_announce_ie_t::set_time(const time_type_t time_type_,
                                  const int32_t N_frames_until_full_sec_,
                                  const int64_t full_sec_) {
    time_type = time_type_;
    N_frames_until_full_sec = N_frames_until_full_sec_;
    full_sec = full_sec_;

    tai_minus_utc_seconds = 0;
}

void time_announce_ie_t::zero() {
    time_type = time_type_t::not_defined;
    N_frames_until_full_sec = common::adt::UNDEFINED_NUMERIC_32;
    full_sec = -1;
    tai_minus_utc_seconds = common::adt::UNDEFINED_NUMERIC_32;
}

bool time_announce_ie_t::is_valid() const {
    if (!common::adt::is_valid<time_type_t>(time_type)) {
        return false;
    }

    if (255 < N_frames_until_full_sec) {
        return false;
    }

    if (full_sec < 0) {
        return false;
    }

    if (255 < tai_minus_utc_seconds) {
        return false;
    }

    return true;
}

uint32_t time_announce_ie_t::get_packed_size() const { return 11; }

void time_announce_ie_t::pack(uint8_t* mac_pdu_offset) const {
    dectnrp_assert(is_valid(), "time_announce_ie_t not valid");

    mac_pdu_offset[0] = static_cast<uint8_t>(std::underlying_type_t<time_type_t>(time_type));
    mac_pdu_offset[1] = static_cast<uint8_t>(N_frames_until_full_sec);
    common::adt::l2b_lower<uint64_t>(&mac_pdu_offset[2], static_cast<uint64_t>(full_sec), 8);
    mac_pdu_offset[10] = static_cast<uint8_t>(tai_minus_utc_seconds);
}

bool time_announce_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    time_type = common::adt::from_coded_value<time_type_t>(mac_pdu_offset[0]);
    N_frames_until_full_sec = mac_pdu_offset[1];
    full_sec = common::adt::b2l_lower<uint64_t>(&mac_pdu_offset[2], 8);
    tai_minus_utc_seconds = mac_pdu_offset[10];

    return is_valid();
}

}  // namespace dectnrp::sp4::extensions
