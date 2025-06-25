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

namespace dectnrp::sp4 {

bool has_valid_inheritance_and_properties(const mmie_t* mmie) {
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
        if (!(mmie->mac_multiplexing_header.mac_ext ==
              mac_multiplexing_header_t::mac_ext_t::No_Length_Field)) {
            return false;
        }
    }

    return true;
}

uint32_t mmie_packing_t::get_packed_size_of_sdu() const { return get_packed_size(); }

uint32_t mmie_packing_t::get_packed_size_of_mmh_sdu() const {
    return mac_multiplexing_header.get_packed_size() + get_packed_size();
}

void mmie_packing_t::pack_mmh_sdu(uint8_t* mac_pdu_offset) {
    mac_multiplexing_header.pack(mac_pdu_offset);
    pack(mac_pdu_offset + mac_multiplexing_header.get_packed_size());
}

uint32_t mmie_flowing_t::get_packed_size_of_sdu() const { return mac_multiplexing_header.length; }

uint32_t mmie_flowing_t::get_packed_size_of_mmh_sdu() const {
    return mac_multiplexing_header.get_packed_size() + mac_multiplexing_header.length;
}

void mmie_flowing_t::pack_mmh_sdu(uint8_t* mac_pdu_offset) {
    mac_multiplexing_header.pack(mac_pdu_offset);
    data_ptr = mac_pdu_offset + mac_multiplexing_header.get_packed_size();
}

void mmie_flowing_t::set_data_size(const uint32_t N_bytes) {
    dectnrp_assert(0 < N_bytes && N_bytes <= (common::adt::bitmask_lsb<16>()),
                   "data size not supported");

    mac_multiplexing_header.mac_ext =
        N_bytes <= common::adt::bitmask_lsb<8>()
            ? mac_multiplexing_header_t::mac_ext_t::Length_Field_8_Bit
            : mac_multiplexing_header_t::mac_ext_t::Length_Field_16_Bit;

    mac_multiplexing_header.length = N_bytes;
}

uint32_t mmie_flowing_t::get_data_size() const { return mac_multiplexing_header.length; }

uint8_t* mmie_flowing_t::get_data_ptr() const {
    dectnrp_assert(data_ptr != nullptr, "nullptr");
    return data_ptr;
}

void mu_depending_t::set_mu(const uint32_t mu_) {
    dectnrp_assert(std::has_single_bit(mu_) && mu_ <= 8, "mu must be 1, 2, 4 or 8");
    mu = mu_;
}

}  // namespace dectnrp::sp4
