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

#include "dectnrp/sections_part4/mac_messages_and_ie/resource_allocation_ie.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part2/channel_arrangement.hpp"

namespace dectnrp::sp4 {

resource_allocation_ie_t::resource_allocation_ie_t() {
    mac_multiplexing_header.zero();
    mac_multiplexing_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_multiplexing_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Resource_Allocation_IE;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void resource_allocation_ie_t::zero() {
    allocation_t allocation;
    allocation.start_subslot = common::adt::UNDEFINED_NUMERIC_32;
    allocation.length_type = length_type_t::length_in_subslots;
    allocation.length = common::adt::UNDEFINED_NUMERIC_32;

    allocation_dl = allocation;
    allocation_dl.reset();

    allocation_ul = allocation;
    allocation_ul.reset();

    is_additional_allocation = false;

    short_rd_id = common::adt::UNDEFINED_NUMERIC_32;
    short_rd_id.reset();

    repeat_info_t r_info;
    r_info.repeat_type = repeat_type_t::repeated_in_following_frames;
    r_info.allow_specific_repeated_resources = false;
    r_info.repetition = common::adt::UNDEFINED_NUMERIC_32;
    r_info.validity = common::adt::UNDEFINED_NUMERIC_32;

    repeat_info = r_info;
    repeat_info.reset();

    sfn_offset = common::adt::UNDEFINED_NUMERIC_32;
    sfn_offset.reset();

    channel = common::adt::UNDEFINED_NUMERIC_32;
    channel.reset();

    dect_scheduled_resource_failure = dect_scheduled_resource_failure_t::not_defined;
    dect_scheduled_resource_failure.reset();
}

bool resource_allocation_ie_t::is_valid() const {
    // in case all previously allocated scheduled resources are released,
    // no other fields are present in this IE
    if (!allocation_dl.has_value() && !allocation_ul.has_value()) {
        return true;
    }

    auto is_allocation_valid = [&mu_ = mu](const allocation_t& allocation) -> bool {
        return allocation.start_subslot <= (mu_ <= 4 ? 0xff : 0xffff) &&
               allocation.length <= common::adt::bitmask_lsb<7>();
    };

    // check validity of DL resources (if included)
    if (const auto& value = allocation_dl.value();
        allocation_dl.has_value() && !is_allocation_valid(value)) {
        return false;
    }

    // check validity of UL resources (if included)
    if (const auto& value = allocation_ul.value();
        allocation_ul.has_value() && !is_allocation_valid(value)) {
        return false;
    }

    // check validity of Short RD-ID (if included)
    if (short_rd_id.has_value() && short_rd_id.value() > 0xffff) {
        return false;
    }

    // check validity of Repetition and Validity fields (if included)
    if (const auto& value = repeat_info.value();
        repeat_info.has_value() &&
        (value.repetition > 0xff || value.repetition == 0 || value.validity > 0xff)) {
        return false;
    }

    // check validity of SFN Offset (if included)
    if (sfn_offset.has_value() && sfn_offset.value() > 0xff) {
        return false;
    }

    // check validity of Channel field (if included)
    if (channel.has_value() && !sp2::is_absolute_channel_number_in_range(channel.value())) {
        return false;
    }

    // check validity of dectScheduledResourceFailure timer value (if included)
    if (dect_scheduled_resource_failure.has_value() &&
        !common::adt::is_valid(dect_scheduled_resource_failure.value())) {
        return false;
    }

    return true;
}

uint32_t resource_allocation_ie_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "resource allocation IE is not valid");

    // the minimum content of the resource allocation IE is 1 octet, when the Allocation Type is set
    // to 00 (i.e., all scheduled resources are released)
    if (!allocation_dl.has_value() && !allocation_ul.has_value()) {
        return 1;
    }

    // otherwise the length of the IE is a minimum of 4 octets
    uint32_t packed_size = mu <= 4 ? 4 : 5;

    if (allocation_dl.has_value() && allocation_ul.has_value()) {
        packed_size += mu <= 4 ? 2 : 3;
    }

    packed_size += short_rd_id.has_value() * 2;
    packed_size += repeat_info.has_value() * 2;
    packed_size += sfn_offset.has_value();
    packed_size += channel.has_value() * 2;
    packed_size += dect_scheduled_resource_failure.has_value();

    return packed_size;
}

void resource_allocation_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that resource allocation IE is valid before packing
    dectnrp_assert(is_valid(), "resource allocation IE is not valid");

    // set Allocation Type field in octet 0
    mac_pdu_offset[0] = allocation_ul.has_value() << 7;
    mac_pdu_offset[0] |= allocation_dl.has_value() << 6;

    // check whether Allocation Type is set to 00
    if (!mac_pdu_offset[0]) {
        // No other fields are present in this IE.
        // Other bits in the bitmap shall be ignored by the RD.
        dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
        dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == 1,
                       "lengths do not match");
        return;
    }

    // set Add and ID fields in octet 0
    mac_pdu_offset[0] |= is_additional_allocation << 5;
    mac_pdu_offset[0] |= short_rd_id.has_value() << 4;

    // set Repeat field in octet 0
    if (const auto& value = repeat_info.value(); !repeat_info.has_value()) {
        mac_pdu_offset[0] |= std::to_underlying(repeat_t::single_allocation) << 1;
    } else if (value.repeat_type == repeat_type_t::repeated_in_following_frames) {
        if (value.allow_specific_repeated_resources) {
            mac_pdu_offset[0] |=
                std::to_underlying(
                    repeat_t::repeated_in_following_frames_allow_specific_repeated_resources)
                << 1;
        } else {
            mac_pdu_offset[0] |= std::to_underlying(repeat_t::repeated_in_following_frames) << 1;
        }
    } else {
        if (value.allow_specific_repeated_resources) {
            mac_pdu_offset[0] |=
                std::to_underlying(
                    repeat_t::repeated_in_following_subslots_allow_specific_repeated_resources)
                << 1;
        } else {
            mac_pdu_offset[0] |= std::to_underlying(repeat_t::repeated_in_following_subslots) << 1;
        }
    }

    // set SFN field in octet 0
    mac_pdu_offset[0] |= sfn_offset.has_value();

    // set Channel and RLF fields in octet 1
    mac_pdu_offset[1] |= channel.has_value() << 7;
    mac_pdu_offset[1] |= dect_scheduled_resource_failure.has_value() << 6;

    uint32_t N_bytes = 2;

    auto pack_resource_allocation = [&mu_ = mu](const allocation_t& src,
                                                uint8_t* const dst) -> uint32_t {
        uint32_t N_bytes_ = mu_ <= 4 ? 1 : 2;
        common::adt::l2b_lower(dst, src.start_subslot, N_bytes_);
        *(dst + N_bytes_) = std::to_underlying(src.length_type) << 7;
        *(dst + N_bytes_) |= src.length;
        return N_bytes_ + 1;
    };

    // set optional fields of downlink resource allocation
    if (allocation_dl.has_value()) {
        N_bytes += pack_resource_allocation(allocation_dl.value(), mac_pdu_offset + N_bytes);
    }

    // set optional fields of uplink resource allocation
    if (allocation_ul.has_value()) {
        N_bytes += pack_resource_allocation(allocation_ul.value(), mac_pdu_offset + N_bytes);
    }

    // pack optional Short RD-ID field
    if (short_rd_id.has_value()) {
        common::adt::l2b_lower(mac_pdu_offset + N_bytes, short_rd_id.value(), 2);
        N_bytes += 2;
    }

    // pack optional Repetition and Validity fields
    if (repeat_info.has_value()) {
        const auto& value = repeat_info.value();
        mac_pdu_offset[N_bytes] = value.repetition;
        mac_pdu_offset[N_bytes + 1] = value.validity;
        N_bytes += 2;
    }

    // pack optional SFN Offset field
    if (sfn_offset.has_value()) {
        mac_pdu_offset[N_bytes] = sfn_offset.value();
        ++N_bytes;
    }

    // pack optional Channel field
    if (channel.has_value()) {
        const auto& value = channel.value();
        mac_pdu_offset[N_bytes] = value >> 8;
        mac_pdu_offset[N_bytes + 1] = value & common::adt::bitmask_lsb<8>();
        N_bytes += 2;
    }

    // pack optional dectScheduledResourceFailure field
    if (dect_scheduled_resource_failure.has_value()) {
        mac_pdu_offset[N_bytes] = std::to_underlying(dect_scheduled_resource_failure.value());
        ++N_bytes;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == N_bytes,
                   "lengths do not match");
}

bool resource_allocation_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    uint32_t N_bytes = 2;

    auto unpack_resource_allocation = [&mu_ = mu](const uint8_t* src,
                                                  allocation_t& dst) -> uint32_t {
        uint32_t N_bytes_ = mu_ <= 4 ? 1 : 2;
        dst.start_subslot = common::adt::b2l_lower(src, N_bytes_);
        dst.length_type = static_cast<length_type_t>(src[N_bytes_] >> 7);
        dst.length = src[N_bytes_] & common::adt::bitmask_lsb<7>();
        return N_bytes_ + 1;
    };

    // unpack optional DL/UL resources
    switch (allocation_t allocation; mac_pdu_offset[0] >> 6) {
        using enum allocation_type_t;

        case std::to_underlying(release_all_scheduled_resources):
            return true;

        case std::to_underlying(dl_resources):
            N_bytes += unpack_resource_allocation(mac_pdu_offset + N_bytes, allocation);
            allocation_dl = allocation;
            break;

        case std::to_underlying(ul_resources):
            N_bytes += unpack_resource_allocation(mac_pdu_offset + N_bytes, allocation);
            allocation_ul = allocation;
            break;

        case std::to_underlying(dl_ul_resources):
            N_bytes += unpack_resource_allocation(mac_pdu_offset + N_bytes, allocation);
            allocation_dl = allocation;
            N_bytes += unpack_resource_allocation(mac_pdu_offset + N_bytes, allocation);
            allocation_ul = allocation;
            break;
    }

    // unpack Add field in octet 0
    is_additional_allocation = (mac_pdu_offset[0] >> 5) & 1;

    // unpack optional Short RD-ID
    if ((mac_pdu_offset[0] >> 4) & 1) {
        short_rd_id = common::adt::b2l_lower(mac_pdu_offset + N_bytes, 2);
        N_bytes += 2;
    }

    // unpack Repeat, Repetition and Validity fields
    switch (repeat_info_t value; (mac_pdu_offset[0] >> 1) & 0b111) {
        using enum repeat_t;

        case std::to_underlying(single_allocation):
            break;

        case std::to_underlying(repeated_in_following_frames):
            value.repeat_type = repeat_type_t::repeated_in_following_frames;
            value.allow_specific_repeated_resources = false;
            value.repetition = *(mac_pdu_offset + N_bytes);
            value.validity = *(mac_pdu_offset + N_bytes + 1);
            repeat_info = value;
            N_bytes += 2;
            break;

        case std::to_underlying(repeated_in_following_subslots):
            value.repeat_type = repeat_type_t::repeated_in_following_subslots;
            value.allow_specific_repeated_resources = false;
            value.repetition = *(mac_pdu_offset + N_bytes);
            value.validity = *(mac_pdu_offset + N_bytes + 1);
            repeat_info = value;
            N_bytes += 2;
            break;

        case std::to_underlying(repeated_in_following_frames_allow_specific_repeated_resources):
            value.repeat_type = repeat_type_t::repeated_in_following_frames;
            value.allow_specific_repeated_resources = true;
            value.repetition = *(mac_pdu_offset + N_bytes);
            value.validity = *(mac_pdu_offset + N_bytes + 1);
            repeat_info = value;
            N_bytes += 2;
            break;

        case std::to_underlying(repeated_in_following_subslots_allow_specific_repeated_resources):
            value.repeat_type = repeat_type_t::repeated_in_following_subslots;
            value.allow_specific_repeated_resources = true;
            value.repetition = *(mac_pdu_offset + N_bytes);
            value.validity = *(mac_pdu_offset + N_bytes + 1);
            repeat_info = value;
            N_bytes += 2;
            break;

        default:
            return false;
    }

    // unpack optional SFN Offset field
    if (mac_pdu_offset[0] & 1) {
        sfn_offset = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack optional Channel field
    if (mac_pdu_offset[1] >> 7) {
        uint32_t value = mac_pdu_offset[N_bytes] << 8;
        value |= *(mac_pdu_offset + N_bytes + 1);
        value &= common::adt::bitmask_lsb<13>();
        channel = value;
        N_bytes += 2;
    }

    // unpack optional dectScheduledResourceFailure field
    if ((mac_pdu_offset[1] >> 6) & 1) {
        dect_scheduled_resource_failure =
            common::adt::from_coded_value<dect_scheduled_resource_failure_t>(
                mac_pdu_offset[N_bytes] & 0xf);
        ++N_bytes;
    }

    dectnrp_assert(get_packed_size() == N_bytes, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t resource_allocation_ie_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t packed_size;

    // check if DL/UL resources are included (Allocation Type field)
    switch (mac_pdu_offset[0] >> 6) {
        using enum allocation_type_t;

        case std::to_underlying(release_all_scheduled_resources):
            return 1;

        case std::to_underlying(dl_resources):
        case std::to_underlying(ul_resources):
            packed_size = mu <= 4 ? 4 : 5;
            break;

        case std::to_underlying(dl_ul_resources):
            packed_size = mu <= 4 ? 6 : 8;
            break;
    }

    // check if Short RD-ID is present
    if ((mac_pdu_offset[0] >> 4) & 1) {
        packed_size += 2;
    }

    // check whether resource allocation is single or repeated allocation (Repeat field)
    switch ((mac_pdu_offset[0] >> 1) & 0b111) {
        using enum repeat_t;

        case std::to_underlying(single_allocation):
            // Repetition and Validity fields are not present
            break;

        case std::to_underlying(repeated_in_following_frames):
        case std::to_underlying(repeated_in_following_subslots):
        case std::to_underlying(repeated_in_following_frames_allow_specific_repeated_resources):
        case std::to_underlying(repeated_in_following_subslots_allow_specific_repeated_resources):
            // Repetition and Validity fields are present
            packed_size += 2;
            break;

        default:
            // values are reserved
            return peek_errc::NONRESERVED_FIELD_SET_TO_RESERVED;
    }

    // check if SFN Offset field is present
    packed_size += mac_pdu_offset[0] & 1;

    // check if Channel field is present
    packed_size += (mac_pdu_offset[1] >> 7) * 2;

    // check if dectScheduledResourceFailure field is present
    packed_size += (mac_pdu_offset[1] >> 6) & 1;

    return packed_size;
}

}  // namespace dectnrp::sp4
