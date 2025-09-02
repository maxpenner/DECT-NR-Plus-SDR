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

#include "dectnrp/sections_part3/transport_block_size.hpp"

#include "dectnrp/common/adt/miscellaneous.hpp"

namespace dectnrp::sp3::transport_block_size {

uint32_t get_G(const uint32_t N_SS, const uint32_t N_PDC_subc, const uint32_t N_bps) {
    return N_SS * N_PDC_subc * N_bps;
}

uint32_t get_N_PDC_bits(const uint32_t N_SS,
                        const uint32_t N_PDC_subc,
                        const uint32_t N_bps,
                        const uint32_t R_numerator,
                        const uint32_t R_denominator) {
    return (get_G(N_SS, N_PDC_subc, N_bps) * R_numerator) / R_denominator;
}

uint32_t get_N_TB_bits(const uint32_t N_SS,
                       const uint32_t N_PDC_subc,
                       const uint32_t N_bps,
                       const uint32_t R_numerator,
                       const uint32_t R_denominator,
                       const uint32_t Z) {
    const uint32_t N_PDC_bits = get_N_PDC_bits(N_SS, N_PDC_subc, N_bps, R_numerator, R_denominator);

    const uint32_t L = 24;

    uint32_t M;
    if (N_PDC_bits <= 512) {
        M = 8;
    } else if (N_PDC_bits <= 1024) {
        M = 16;
    } else if (N_PDC_bits <= 2048) {
        M = 32;
    } else {
        M = 64;
    }

    const uint32_t N_M = (N_PDC_bits / M) * M;

    // this can happen, we indicate ill-configure TB by returning zero
    if (N_M == 0) {
        return 0;
    }
    if (N_M <= L) {
        return 0;
    }

    uint32_t N_TB_bits = 0;

    if (N_M <= Z) {
        N_TB_bits = N_M - L;
    } else {
        uint32_t C = common::adt::ceil_divide_integer(N_M - L, Z);
        N_TB_bits = N_M - (C + 1) * L;
    }

    return N_TB_bits;
}

}  // namespace dectnrp::sp3::transport_block_size
