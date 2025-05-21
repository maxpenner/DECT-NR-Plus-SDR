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

#include "dectnrp/sections_part4/mac_messages_and_ie/padding_ie.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

padding_ie_t::padding_ie_t() { dectnrp_assert(check_validity_at_runtime(this), "mmie invalid"); }

void padding_ie_t::set_nof_padding_bytes(const uint32_t N_bytes) {
    dectnrp_assert(1 <= N_bytes && N_bytes <= N_PADDING_BYTES_MAX,
                   "invalid number of padding bytes");

    switch (N_bytes) {
        // one octet of padding is needed
        case 1:
            mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::Length_Field_1_Bit;
            mac_mux_header.ie_type.mac_ext_11_len_0 =
                mac_multiplexing_header_t::ie_type_mac_ext_11_len_0_t::Padding_IE;
            mac_mux_header.length = 0;
            break;

        // two octets of padding are needed
        case 2:
            mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::Length_Field_1_Bit;
            mac_mux_header.ie_type.mac_ext_11_len_1 =
                mac_multiplexing_header_t::ie_type_mac_ext_11_len_1_t::Padding_IE;
            mac_mux_header.length = 1;
            break;

        // more than two octets of padding are needed
        default:
            mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::Length_Field_8_Bit;
            mac_mux_header.ie_type.mac_ext_00_01_10 =
                mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Padding_IE;
            mac_mux_header.length = N_bytes - mac_mux_header.get_packed_size();
            break;
    }
}

}  // namespace dectnrp::sp4
