/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/sections_part4/mac_messages_and_ie/configuration_request_ie.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

configuration_request_ie_t::configuration_request_ie_t() {
    mac_multiplexing_header.zero();
    mac_multiplexing_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::Length_Field_1_Bit;
    mac_multiplexing_header.length = 0;
    mac_multiplexing_header.ie_type.mac_ext_11_len_0 =
        mac_multiplexing_header_t::ie_type_mac_ext_11_len_0_t::Configuration_Request_IE;

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

}  // namespace dectnrp::sp4
