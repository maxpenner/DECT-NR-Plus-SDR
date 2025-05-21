/*
 * Copyright 2023-2025 Maxim Penner
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

#include "dectnrp/sections_part3/transmission_packet_structure.hpp"

#include "dectnrp/constants.hpp"
#include "dectnrp/sections_part3/pdc.hpp"

namespace dectnrp::sp3::transmission_packet_structure {

uint32_t get_N_PACKET_symb(const uint32_t PacketLengthType,
                           const uint32_t PacketLength,
                           const uint32_t N_SLOT_u_symb,
                           const uint32_t N_SLOT_u_subslot) {
    return (PacketLengthType == 0) ? PacketLength * N_SLOT_u_symb / N_SLOT_u_subslot
                                   : PacketLength * N_SLOT_u_symb;
}

uint32_t get_N_PACKET_symb(const uint32_t PacketLengthType,
                           const uint32_t PacketLength,
                           const numerologies_t& q) {
    return get_N_PACKET_symb(PacketLengthType, PacketLength, q.N_SLOT_u_symb, q.N_SLOT_u_subslot);
}

uint32_t get_N_samples_OFDM_symbol(const uint32_t b) {
    // Table 4.3-1
    // 64 + 8 = 72
    return constants::N_b_DFT_CP_min_u_b * b;
}

uint32_t get_N_samples_OFDM_symbol_CP_only(const uint32_t b) {
    return constants::N_b_CP_min_u_b * b;
}

uint32_t get_N_samples_OFDM_symbol_without_CP(const uint32_t b) {
    return constants::N_b_DFT_min_u_b * b;
}

uint32_t get_N_samples_STF(const uint32_t u, const uint32_t b) {
    uint32_t N_samples_STF = 0;

    const uint32_t N_samples_OFDM_symbol = get_N_samples_OFDM_symbol(b);

    if (u == 1) {
        // N_samples_OFDM_symbol is (64+8)*b and therefore a multiple of 9
        N_samples_STF = (N_samples_OFDM_symbol * 14) / 9;
    } else if (u == 2 || u == 4 || u == 8) {
        N_samples_STF = N_samples_OFDM_symbol * 2;
    }

    return N_samples_STF;
}

uint32_t get_N_samples_STF_CP_only(const uint32_t u, const uint32_t b) {
    const uint32_t N_samples_STF = get_N_samples_STF(u, b);

    return N_samples_STF - constants::N_b_DFT_min_u_b * b;
}

uint32_t get_N_samples_DF(const uint32_t u, const uint32_t b, const uint32_t N_PACKET_symb) {
    return get_N_samples_OFDM_symbol(b) * pdc_t::get_N_DF_symb(u, N_PACKET_symb);
}

uint32_t get_N_samples_GI(const uint32_t u, const uint32_t b) {
    uint32_t N_samples_GI = 0;

    const uint32_t N_samples_OFDM_symbol = get_N_samples_OFDM_symbol(b);

    if (u == 1) {
        // N_samples_OFDM_symbol is (64+8)*b and therefore a multiple of 9
        N_samples_GI = (N_samples_OFDM_symbol * 4) / 9;
    } else if (u == 2 || u == 4) {
        N_samples_GI = N_samples_OFDM_symbol;
    } else if (u == 8) {
        N_samples_GI = N_samples_OFDM_symbol * 2;
    }

    return N_samples_GI;
}

}  // namespace dectnrp::sp3::transmission_packet_structure
