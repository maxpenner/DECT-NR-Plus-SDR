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

#include "dectnrp/sections_part4/mac_messages_and_ie/load_info_ie.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::section4 {

load_info_ie_t::load_info_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Load_Info_IE;

    zero();

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void load_info_ie_t::zero() {
    max_assoc = max_assoc_size_t::not_defined;
    rd_pt_load_percentage = std::nullopt;
    rach_load_percentage = std::nullopt;
    channel_load = std::nullopt;
    traffic_load_percentage = common::adt::UNDEFINED_NUMERIC_32;
    max_nof_associated_rd = common::adt::UNDEFINED_NUMERIC_32;
    rd_ft_load_percentage = common::adt::UNDEFINED_NUMERIC_32;
}

bool load_info_ie_t::is_valid() const {
    if (!common::adt::is_valid(max_assoc)) {
        return false;
    }

    if (rd_pt_load_percentage && *rd_pt_load_percentage > 0xff) {
        return false;
    }

    if (rach_load_percentage && *rach_load_percentage > 0xff) {
        return false;
    }

    if (channel_load && (channel_load->percentage_subslots_free > 0xff ||
                         channel_load->percentage_subslots_busy > 0xff)) {
        return false;
    }

    if (traffic_load_percentage > 0xff) {
        return false;
    }

    if (max_nof_associated_rd > (max_assoc == max_assoc_size_t::_8_bit ? 0xff : 0xffff)) {
        return false;
    }

    return rd_ft_load_percentage <= 0xff;
}

uint32_t load_info_ie_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "Load Info IE is not valid");

    uint32_t packed_size = max_assoc == max_assoc_size_t::_8_bit ? 4 : 5;
    packed_size += rd_pt_load_percentage.has_value();
    packed_size += rach_load_percentage.has_value();
    packed_size += channel_load.has_value() * 2;
    return packed_size;
}

void load_info_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that IE is valid before packing
    dectnrp_assert(is_valid(), "Load Info IE is not valid");

    // pack mandatory fields in octet 0 of IE
    mac_pdu_offset[0] = std::to_underlying(max_assoc) << 3;
    mac_pdu_offset[0] |= rd_pt_load_percentage.has_value() << 2;
    mac_pdu_offset[0] |= rach_load_percentage.has_value() << 1;
    mac_pdu_offset[0] |= channel_load.has_value();

    // pack TRAFFIC LOAD PERCENTAGE field in octet 1 of IE
    mac_pdu_offset[1] = traffic_load_percentage;

    // pack MAX NUMBER ASSOCIATED RDS field in octets 2 (and 3) of IE
    uint32_t N_bytes = max_assoc == max_assoc_size_t::_8_bit ? 1 : 2;
    common::adt::l2b_lower(mac_pdu_offset + 2, max_nof_associated_rd, N_bytes);
    N_bytes += 2;

    // pack CURRENTLY ASSOCIATED RDS IN FT MODE PERCENTAGE field
    mac_pdu_offset[N_bytes] = rd_ft_load_percentage;
    ++N_bytes;

    // pack CURRENTLY ASSOCIATED RDS IN PT MODE PERCENTAGE field (if present)
    if (rd_pt_load_percentage) {
        mac_pdu_offset[N_bytes] = *rd_pt_load_percentage;
        ++N_bytes;
    }

    // pack RACH LOAD IN PERCENTAGE field (if present)
    if (rach_load_percentage) {
        mac_pdu_offset[N_bytes] = *rach_load_percentage;
        ++N_bytes;
    }

    // pack channel load fields (if present)
    if (channel_load) {
        *(mac_pdu_offset + N_bytes) = channel_load->percentage_subslots_free;
        *(mac_pdu_offset + N_bytes + 1) = channel_load->percentage_subslots_busy;
        N_bytes += 2;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == N_bytes,
                   "lengths do not match");
}

bool load_info_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack MAX ASSOC field in octet 0 of IE
    max_assoc = common::adt::from_coded_value<max_assoc_size_t>((mac_pdu_offset[0] >> 3) & 1);

    // unpack TRAFFIC LOAD PERCENTAGE field in octet 1 of IE
    traffic_load_percentage = mac_pdu_offset[1];

    // unpack MAX NUMBER ASSOCIATED RDS field in octets 2 (and 3) of IE
    uint32_t N_bytes = max_assoc == max_assoc_size_t::_8_bit ? 1 : 2;
    max_nof_associated_rd = common::adt::b2l_lower(mac_pdu_offset + 2, N_bytes);
    N_bytes += 2;

    // unpack CURRENTLY ASSOCIATED RDS IN FT MODE PERCENTAGE field
    rd_ft_load_percentage = mac_pdu_offset[N_bytes];
    ++N_bytes;

    // unpack CURRENTLY ASSOCIATED RDS IN PT MODE PERCENTAGE field (if present)
    if (mac_pdu_offset[0] & 4) {
        rd_pt_load_percentage = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack RACH LOAD IN PERCENTAGE field (if present)
    if (mac_pdu_offset[0] & 2) {
        rach_load_percentage = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack channel load fields (if present)
    if (mac_pdu_offset[0] & 1) {
        channel_load = {.percentage_subslots_free = *(mac_pdu_offset + N_bytes),
                        .percentage_subslots_busy = *(mac_pdu_offset + N_bytes + 1)};
        N_bytes += 2;
    }

    dectnrp_assert(get_packed_size() == N_bytes, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t load_info_ie_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t packed_size =
        (mac_pdu_offset[0] & 8) == std::to_underlying(max_assoc_size_t::_8_bit) ? 4 : 5;
    packed_size += mac_pdu_offset[0] & 4;
    packed_size += mac_pdu_offset[0] & 2;
    packed_size += (mac_pdu_offset[0] & 1) * 2;
    return packed_size;
}

}  // namespace dectnrp::section4
