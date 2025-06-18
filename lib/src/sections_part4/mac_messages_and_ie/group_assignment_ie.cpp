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

#include "dectnrp/sections_part4/mac_messages_and_ie/group_assignment_ie.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

group_assignment_ie_t::group_assignment_ie_t() {
    mac_multiplexing_header.zero();
    mac_multiplexing_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Group_Assignment_IE;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void group_assignment_ie_t::zero() {
    single = nof_resource_assignments_t::not_defined;
    group_id = common::adt::UNDEFINED_NUMERIC_32;
    resource_assignments.clear();
}

bool group_assignment_ie_t::is_valid() const {
    if (!common::adt::is_valid(single)) {
        return false;
    }

    if (single == nof_resource_assignments_t::single_assignment &&
        resource_assignments.size() != 1) {
        return false;
    }

    if (single == nof_resource_assignments_t::multiple_assignments &&
        resource_assignments.size() < 2) {
        return false;
    }

    if (group_id > common::adt::bitmask_lsb<7>()) {
        return false;
    }

    return std::all_of(
        resource_assignments.cbegin(), resource_assignments.cend(), [](const auto& assignment) {
            return common::adt::is_valid(assignment.direct) &&
                   assignment.resource_tag <= common::adt::bitmask_lsb<7>();
        });
}

uint32_t group_assignment_ie_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "Group Assignment IE is not valid");

    return 1 + resource_assignments.size();
}

void group_assignment_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that IE is valid before packing
    dectnrp_assert(is_valid(), "Group Assignment IE is not valid");

    // pack SINGLE and GROUP ID fields in octet 0 of IE
    mac_pdu_offset[0] = std::to_underlying(single) << 7;
    mac_pdu_offset[0] |= group_id;

    // pack resource assignments starting in octet 1 of IE
    for (uint32_t N_bytes = 1; const auto& assignment : resource_assignments) {
        mac_pdu_offset[N_bytes] = std::to_underlying(assignment.direct) << 7;
        mac_pdu_offset[N_bytes] |= assignment.resource_tag;
        ++N_bytes;
    }
}

bool group_assignment_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack SINGLE and GROUP ID fields in octet 0 of IE
    single = common::adt::from_coded_value<nof_resource_assignments_t>(mac_pdu_offset[0] >> 7);
    group_id = mac_pdu_offset[0] & common::adt::bitmask_lsb<7>();

    // unpack resource assignments starting in octet 1 of IE
    for (uint32_t N_bytes = 1; N_bytes < mac_multiplexing_header.length; ++N_bytes) {
        resource_assignment_t assignment;
        assignment.direct =
            common::adt::from_coded_value<resource_direction_t>(mac_pdu_offset[N_bytes] >> 7);
        assignment.resource_tag = mac_pdu_offset[N_bytes] & common::adt::bitmask_lsb<7>();
        resource_assignments.push_back(assignment);
    }

    dectnrp_assert(get_packed_size() == mac_multiplexing_header.length, "lengths do not match");

    return is_valid();
}

}  // namespace dectnrp::sp4
