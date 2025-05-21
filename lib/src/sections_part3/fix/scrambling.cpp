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

#include "dectnrp/sections_part3/fix/scrambling.hpp"

extern "C" {
#include "srsran/phy/utils/bit.h"
#include "srsran/phy/utils/vector.h"
}

namespace dectnrp::sp3::fix {

// this function is also static in srsrRAN so it has to be added here
static inline void scrambling_b(uint8_t* c, uint8_t* data, int len) {
    srsran_vec_xor_bbb(c, data, data, len);
}

void srsran_scrambling_bytes_with_sequence_offset_FIX(srsran_sequence_t* s,
                                                      uint8_t* data,
                                                      int len,
                                                      int offset) {
    const int virtual_full_length = offset + len;

    // scramble bits if offset is not a full byte
    if (offset % 8 > 0) {
        uint8_t tmp_bits[8];
        srsran_bit_unpack_vector(&data[offset / 8], tmp_bits, 8);
        scrambling_b(&s->c[offset], &tmp_bits[offset % 8], 8 - offset % 8);
        srsran_bit_pack_vector(tmp_bits, &data[offset / 8], 8);

        len -= 8 - offset % 8;          // reduce length by freshly scrambled bits
        offset = (offset / 8 + 1) * 8;  // set offset to next full byte
    }

    scrambling_b(&s->c_bytes[offset / 8], &data[offset / 8], len / 8);

    // scramble last bits
    if (len % 8 > 0) {
        uint8_t tmp_bits[8];
        srsran_bit_unpack_vector(&data[virtual_full_length / 8], tmp_bits, len % 8);
        scrambling_b(&s->c[8 * (virtual_full_length / 8)], tmp_bits, len % 8);
        srsran_bit_pack_vector(tmp_bits, &data[virtual_full_length / 8], len % 8);
    }
}

}  // namespace dectnrp::sp3::fix
