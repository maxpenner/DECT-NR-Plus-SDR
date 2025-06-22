/*
 * Copyright 2023-2024 Maxim Penner, Alexander Poets
 * Copyright 2025-2025 Maxim Penner
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

#include "dectnrp/sections_part4/mac_messages_and_ie/random_access_resource_ie.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part2/channel_arrangement.hpp"

namespace dectnrp::sp4 {

random_access_resource_ie_t::random_access_resource_ie_t() {
    mac_multiplexing_header.zero();
    mac_multiplexing_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_multiplexing_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Random_Access_Resource_IE;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void random_access_resource_ie_t::zero() {
    repeat_info = {.repetition = common::adt::UNDEFINED_NUMERIC_32,
                   .validity = common::adt::UNDEFINED_NUMERIC_32};
    repeat_info.reset();

    sfn_offset = common::adt::UNDEFINED_NUMERIC_32;
    sfn_offset.reset();

    channel = common::adt::UNDEFINED_NUMERIC_32;
    channel.reset();

    channel_2 = common::adt::UNDEFINED_NUMERIC_32;
    channel_2.reset();

    allocation.start_subslot = common::adt::UNDEFINED_NUMERIC_32;
    allocation.length = common::adt::UNDEFINED_NUMERIC_32;

    max_rach_length = common::adt::UNDEFINED_NUMERIC_32;
    cw_min = cw_min_t::not_defined;
    response_window_length = common::adt::UNDEFINED_NUMERIC_32;
    cw_max = cw_max_t::not_defined;
}

bool random_access_resource_ie_t::is_valid() const {
    // check validity of Repetition and Validity fields (if included)
    if (const auto& value = repeat_info.value();
        repeat_info.has_value() &&
        (value.repetition > 0xff || value.repetition == 0 || value.validity > 0xff)) {
        return false;
    }

    // check validity of SFN Offset field (if included)
    if (sfn_offset.has_value() && sfn_offset.value() > 0xff) {
        return false;
    }

    // check validity of channel field (if included)
    if (channel.has_value() && !sp2::is_absolute_channel_number_in_range(channel.value())) {
        return false;
    }

    // check validity of separate channel field (if included)
    if (channel_2.has_value() && !sp2::is_absolute_channel_number_in_range(channel_2.value())) {
        return false;
    }

    // check validity of RACH resource allocation
    if (allocation.start_subslot > (mu <= 4 ? 0xff : 0xffff) ||
        allocation.length > common::adt::bitmask_lsb<7>()) {
        return false;
    }

    // check validity of remaining non-optional fields
    if (max_rach_length > 0xf || !common::adt::is_valid(cw_min) || response_window_length > 0xf ||
        !common::adt::is_valid(cw_max)) {
        return false;
    }

    return true;
}

uint32_t random_access_resource_ie_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "RACH resource IE is not valid");

    uint32_t packed_size = mu <= 4 ? 5 : 6;
    packed_size += repeat_info.has_value() * 2;
    packed_size += sfn_offset.has_value();
    packed_size += channel.has_value() * 2;
    packed_size += channel_2.has_value() * 2;

    return packed_size;
}

void random_access_resource_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that RACH resource IE is valid before packing
    dectnrp_assert(is_valid(), "RACH resource IE is not valid");

    // set SFN, Channel and Chan_2 fields in octet 0
    mac_pdu_offset[0] = sfn_offset.has_value() << 2;
    mac_pdu_offset[0] |= channel.has_value() << 1;
    mac_pdu_offset[0] |= channel_2.has_value();

    // set Start Subslot field starting in octet 1
    common::adt::l2b_lower(mac_pdu_offset + 1, allocation.start_subslot, mu <= 4 ? 1 : 2);

    uint32_t N_bytes = mu <= 4 ? 2 : 3;

    // pack Length Type and Length fields
    mac_pdu_offset[N_bytes] = std::to_underlying(allocation.length_type) << 7;
    mac_pdu_offset[N_bytes] |= allocation.length;
    ++N_bytes;

    // pack MAX Len Type, MAX RACH Length and CW Min Sig fields
    mac_pdu_offset[N_bytes] = std::to_underlying(max_rach_length_type) << 7;
    mac_pdu_offset[N_bytes] |= max_rach_length << 3;
    mac_pdu_offset[N_bytes] |= std::to_underlying(cw_min);
    ++N_bytes;

    // pack DECT Delay, Response Window and CW Max Sig fields
    mac_pdu_offset[N_bytes] = std::to_underlying(dect_delay) << 7;
    mac_pdu_offset[N_bytes] |= response_window_length << 3;
    mac_pdu_offset[N_bytes] |= std::to_underlying(cw_max);
    ++N_bytes;

    // set Repeat field in octet 0 and pack Repetition and Validity fields if present
    if (const auto& value = repeat_info.value(); !repeat_info.has_value()) {
        mac_pdu_offset[0] |= std::to_underlying(repeat_t::single_allocation) << 3;
    } else {
        mac_pdu_offset[0] |=
            std::to_underlying(
                value.repeat_type ==
                        resource_allocation_ie_t::repeat_type_t::repeated_in_following_frames
                    ? repeat_t::repeated_in_following_frames
                    : repeat_t::repeated_in_following_subslots)
            << 3;

        *(mac_pdu_offset + N_bytes) = value.repetition;
        *(mac_pdu_offset + N_bytes + 1) = value.validity;
        N_bytes += 2;
    }

    // pack SFN Offset field if present
    if (sfn_offset.has_value()) {
        mac_pdu_offset[N_bytes] = sfn_offset.value();
        ++N_bytes;
    }

    auto pack_channel_field = [](const uint32_t src, uint8_t* const dst) -> uint32_t {
        *dst = src >> 8;
        *(dst + 1) = src & 0xff;
        return 2;
    };

    // pack channel field if present
    if (channel.has_value()) {
        N_bytes += pack_channel_field(channel.value(), mac_pdu_offset + N_bytes);
    }

    // pack separate channel field if present
    if (channel_2.has_value()) {
        N_bytes += pack_channel_field(channel_2.value(), mac_pdu_offset + N_bytes);
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == N_bytes,
                   "lengths do not match");
}

bool random_access_resource_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack Start Subslot field starting in octet 1
    allocation.start_subslot = common::adt::b2l_lower(mac_pdu_offset + 1, mu <= 4 ? 1 : 2);

    uint32_t N_bytes = mu <= 4 ? 2 : 3;

    // unpack Length Type and Length fields
    allocation.length_type =
        static_cast<resource_allocation_ie_t::length_type_t>(mac_pdu_offset[N_bytes] >> 7);
    allocation.length = mac_pdu_offset[N_bytes] & common::adt::bitmask_lsb<7>();
    ++N_bytes;

    // unpack MAX Len Type, MAX RACH Length and CW Min Sig fields
    max_rach_length_type =
        static_cast<resource_allocation_ie_t::length_type_t>(mac_pdu_offset[N_bytes] >> 7);
    max_rach_length = (mac_pdu_offset[N_bytes] >> 3) & 0xf;
    cw_min = common::adt::from_coded_value<cw_min_t>(mac_pdu_offset[N_bytes] & 0b111);
    ++N_bytes;

    // unpack DECT Delay, Response Window and CW Max Sig fields
    dect_delay = static_cast<dect_delay_t>(mac_pdu_offset[N_bytes] >> 7);
    response_window_length = (mac_pdu_offset[N_bytes] >> 3) & 0xf;
    cw_max = common::adt::from_coded_value<cw_max_t>(mac_pdu_offset[N_bytes] & 0b111);
    ++N_bytes;

    auto unpack_repetition_validity_fields = [](const uint8_t* src,
                                                repeat_info_t& dst) -> uint32_t {
        dst.repetition = *src;
        dst.validity = *(src + 1);
        return 2;
    };

    // unpack Repetition and Validity fields if present
    switch (repeat_info_t value; (mac_pdu_offset[0] >> 3) & 0b11) {
        using enum repeat_t;

        case std::to_underlying(single_allocation):
            break;

        case std::to_underlying(repeated_in_following_frames):
            value.repeat_type =
                resource_allocation_ie_t::repeat_type_t::repeated_in_following_frames;
            N_bytes += unpack_repetition_validity_fields(mac_pdu_offset + N_bytes, value);
            repeat_info = value;
            break;

        case std::to_underlying(repeated_in_following_subslots):
            value.repeat_type =
                resource_allocation_ie_t::repeat_type_t::repeated_in_following_subslots;
            N_bytes += unpack_repetition_validity_fields(mac_pdu_offset + N_bytes, value);
            repeat_info = value;
            break;

        default:
            return false;
    }

    // unpack SFN Offset field if present
    if (mac_pdu_offset[0] & 0b100) {
        sfn_offset = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    auto unpack_channel_field = [](const uint8_t* src) -> uint32_t {
        uint32_t channel_ = (*src & common::adt::bitmask_lsb<5>()) << 8;
        channel_ |= *(src + 1);
        return channel_;
    };

    // unpack channel field if present
    if (mac_pdu_offset[0] & 0b10) {
        channel = unpack_channel_field(mac_pdu_offset + N_bytes);
        N_bytes += 2;
    }

    // unpack separate channel field if present
    if (mac_pdu_offset[0] & 1) {
        channel_2 = unpack_channel_field(mac_pdu_offset + N_bytes);
        N_bytes += 2;
    }

    dectnrp_assert(get_packed_size() == N_bytes, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t random_access_resource_ie_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t packed_size = mu <= 4 ? 5 : 6;

    // check whether resource allocation is single or repeated allocation (Repeat field)
    switch ((mac_pdu_offset[0] >> 3) & 0b11) {
        using enum repeat_t;

        case std::to_underlying(single_allocation):
            // Repetition and Validity fields are not present
            break;

        case std::to_underlying(repeated_in_following_frames):
        case std::to_underlying(repeated_in_following_subslots):
            // Repetition and Validity fields are present
            packed_size += 2;
            break;

        default:
            // values are reserved
            return peek_errc::NONRESERVED_FIELD_SET_TO_RESERVED;
    }

    // check if SFN Offset field is present
    packed_size += mac_pdu_offset[0] & 0b100;

    // check if channel field is present
    packed_size += (mac_pdu_offset[0] & 0b10) * 2;

    // check if separate channel field is present
    packed_size += (mac_pdu_offset[0] & 1) * 2;

    return packed_size;
}

}  // namespace dectnrp::sp4
