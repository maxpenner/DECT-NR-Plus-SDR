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

#include "dectnrp/sections_part4/mac_messages_and_ie/higher_layer_signalling.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

higher_layer_signalling_t::higher_layer_signalling_t() {
    mac_mux_header.zero();
    data_ptr = nullptr;

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void higher_layer_signalling_t::set_flow_id(const uint32_t flow_id) {
    switch (flow_id) {
        using enum mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t;

        case 1:
            mac_mux_header.ie_type.mac_ext_00_01_10 = Higher_Layer_Signalling_Flow_1;
            break;

        case 2:
            mac_mux_header.ie_type.mac_ext_00_01_10 = Higher_Layer_Signalling_Flow_2;
            break;

        default:
            dectnrp_assert_failure("flow ID must be between 1 and 2");
    }
}

uint32_t higher_layer_signalling_t::get_flow_id() const {
    switch (mac_mux_header.ie_type.mac_ext_00_01_10) {
        using enum mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t;

        case Higher_Layer_Signalling_Flow_1:
            return 1;

        case Higher_Layer_Signalling_Flow_2:
            return 2;

        default:
            return common::adt::UNDEFINED_NUMERIC_32;
    }
}

}  // namespace dectnrp::sp4
