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

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section4 {

bool check_validity_at_runtime(const mmie_t* mmie) {
    if (dynamic_cast<const mmie_packing_t*>(mmie) != nullptr &&
        dynamic_cast<const mmie_flowing_t*>(mmie) != nullptr) {
        return false;
    }

    if (dynamic_cast<const mmie_packing_peeking_t*>(mmie) != nullptr &&
        dynamic_cast<const mmie_flowing_t*>(mmie) != nullptr) {
        return false;
    }

    if (dynamic_cast<const mmie_flowing_t*>(mmie) != nullptr &&
        dynamic_cast<const mu_depending_t*>(mmie) != nullptr) {
        return false;
    }

    // if peeking ...
    if (dynamic_cast<const mmie_packing_peeking_t*>(mmie) != nullptr) {
        // ... an mmie must use No_Length_Field
        if (!(mmie->mac_mux_header.mac_ext ==
              mac_multiplexing_header_t::mac_ext_t::No_Length_Field)) {
            return false;
        }
    }

    return true;
}

uint32_t mmie_packing_t::get_packed_size_of_mmh_sdu() const {
    uint32_t packed_size = mmie_t::get_packed_size_of_mmh_sdu();
    packed_size += get_packed_size();
    return packed_size;
}

void mmie_packing_t::pack_mmh_sdu(uint8_t* mac_pdu_offset) {
    mmie_t::pack_mmh_sdu(mac_pdu_offset);

    uint8_t* dst = mac_pdu_offset + mac_mux_header.get_packed_size();

    pack(dst);
}

uint32_t mmie_flowing_t::get_packed_size_of_mmh_sdu() const {
    uint32_t packed_size = mmie_t::get_packed_size_of_mmh_sdu();
    packed_size += mac_mux_header.length;
    return packed_size;
}

void mmie_flowing_t::pack_mmh_sdu(uint8_t* mac_pdu_offset) {
    mmie_t::pack_mmh_sdu(mac_pdu_offset);

    uint8_t* dst = mac_pdu_offset + mac_mux_header.get_packed_size();

    data_ptr = dst;
}

void mmie_flowing_t::set_data_size(const uint32_t N_bytes) {
    dectnrp_assert(0 < N_bytes && N_bytes <= (common::adt::bitmask_lsb<16>()),
                   "data size is not supported");

    mac_mux_header.mac_ext = N_bytes <= common::adt::bitmask_lsb<8>()
                                 ? mac_multiplexing_header_t::mac_ext_t::Length_Field_8_Bit
                                 : mac_multiplexing_header_t::mac_ext_t::Length_Field_16_Bit;

    mac_mux_header.length = N_bytes;
}

uint32_t mmie_flowing_t::get_data_size() const { return mac_mux_header.length; }

uint8_t* mmie_flowing_t::get_data_ptr() const {
    dectnrp_assert(data_ptr != nullptr, "pointer nullptr");
    return data_ptr;
}

}  // namespace dectnrp::section4
