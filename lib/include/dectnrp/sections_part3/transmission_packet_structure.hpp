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

#pragma once

#include <cstdint>

#include "dectnrp/sections_part3/numerologies.hpp"

namespace dectnrp::sp3::transmission_packet_structure {

/// N_SLOT_u_subslot actually not required since N_SLOT_u_symb/N_SLOT_u_subslot is always 5
uint32_t get_N_PACKET_symb(const uint32_t PacketLengthType,
                           const uint32_t PacketLength,
                           const uint32_t N_SLOT_u_symb,
                           const uint32_t N_SLOT_u_subslot);

uint32_t get_N_PACKET_symb(const uint32_t PacketLengthType,
                           const uint32_t PacketLength,
                           const numerologies_t& q);

uint32_t get_N_samples_OFDM_symbol(const uint32_t b);
uint32_t get_N_samples_OFDM_symbol_CP_only(const uint32_t b);
uint32_t get_N_samples_OFDM_symbol_without_CP(const uint32_t b);

uint32_t get_N_samples_STF(const uint32_t u, const uint32_t b);
uint32_t get_N_samples_STF_CP_only(const uint32_t u, const uint32_t b);

uint32_t get_N_samples_DF(const uint32_t u, const uint32_t b, const uint32_t N_PACKET_symb);

uint32_t get_N_samples_GI(const uint32_t u, const uint32_t b);

}  // namespace dectnrp::sp3::transmission_packet_structure
