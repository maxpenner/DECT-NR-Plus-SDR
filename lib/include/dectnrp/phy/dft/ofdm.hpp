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

#pragma once

#include <cstdint>
#include <string>

extern "C" {
#include "srsran/config.h"
#include "srsran/phy/dft/dft.h"
}

namespace dectnrp::phy::dft {

using ofdm_t = struct ofdm_t {
        uint32_t N_b_DFT_os;
        srsran_dft_plan_t plan_tx;
        srsran_dft_plan_t plan_rx;
};

void get_ofdm(ofdm_t& q, const uint32_t N_b_DFT_os);
void free_ofdm(ofdm_t& q);

/// computes the ifft and copies the cyclic prefix
void single_symbol_tx_ofdm(ofdm_t& q, const cf_t* in, cf_t* out, const uint32_t N_b_CP_os);
void single_symbol_tx_ofdm_zero_copy(ofdm_t& q,
                                     const cf_t* in,
                                     cf_t* out,
                                     const uint32_t N_b_CP_os);

/// removes the cyclic prefix and computes fft
void single_symbol_rx_ofdm(ofdm_t& q, const cf_t* in, cf_t* out, const uint32_t N_b_CP_os);
void single_symbol_rx_ofdm_zero_copy(ofdm_t& q,
                                     const cf_t* in,
                                     cf_t* out,
                                     const uint32_t N_b_CP_os);

void mem_mirror(cf_t* in, const uint32_t len);

}  // namespace dectnrp::phy::dft
