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

#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section4 {

user_plane_data_t::user_plane_data_t() {
    mac_mux_header.zero();
    data_ptr = nullptr;

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void user_plane_data_t::set_flow_id(const uint32_t flow_id) {
    switch (flow_id) {
        using enum mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t;

        case 1:
            mac_mux_header.ie_type.mac_ext_00_01_10 = User_Plane_Data_Flow_1;
            break;

        case 2:
            mac_mux_header.ie_type.mac_ext_00_01_10 = User_Plane_Data_Flow_2;
            break;

        case 3:
            mac_mux_header.ie_type.mac_ext_00_01_10 = User_Plane_Data_Flow_3;
            break;

        case 4:
            mac_mux_header.ie_type.mac_ext_00_01_10 = User_Plane_Data_Flow_4;
            break;

        default:
            dectnrp_assert_failure("flow ID must be between 1 and 4");
    }
}

uint32_t user_plane_data_t::get_flow_id() const {
    switch (mac_mux_header.ie_type.mac_ext_00_01_10) {
        using enum mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t;

        case User_Plane_Data_Flow_1:
            return 1;

        case User_Plane_Data_Flow_2:
            return 2;

        case User_Plane_Data_Flow_3:
            return 3;

        case User_Plane_Data_Flow_4:
            return 4;

        default:
            return common::adt::UNDEFINED_NUMERIC_32;
    }
}

}  // namespace dectnrp::section4
