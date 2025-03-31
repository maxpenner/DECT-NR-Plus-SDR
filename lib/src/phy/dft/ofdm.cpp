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

#include "dectnrp/phy/dft/ofdm.hpp"

#include <cstring>

#define OFDM_MEM_MIRROR_ELEMENTWISE
#ifdef OFDM_MEM_MIRROR_ELEMENTWISE
#else
extern "C" {
#include "srsran/phy/utils/vector.h"
}
#include <vector>
#endif

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::dft {

void get_ofdm(ofdm_t& q, const uint32_t N_b_DFT_os) {
    q.N_b_DFT_os = N_b_DFT_os;

    if (srsran_dft_plan_c(&q.plan_tx, N_b_DFT_os, SRSRAN_DFT_BACKWARD)) {
        dectnrp_assert_failure("unable to set DFT plan");
    }
    if (srsran_dft_plan_c(&q.plan_rx, N_b_DFT_os, SRSRAN_DFT_FORWARD)) {
        dectnrp_assert_failure("unable to set DFT plan");
    }
}

void free_ofdm(ofdm_t& q) {
    srsran_dft_plan_free(&q.plan_tx);
    srsran_dft_plan_free(&q.plan_rx);
}

void single_symbol_tx_ofdm(ofdm_t& q, const cf_t* in, cf_t* out, const uint32_t N_b_CP_os) {
    // ifft
    srsran_dft_run_c(&q.plan_tx, in, &out[N_b_CP_os]);

    // cp
    memcpy(out, &out[N_b_CP_os], N_b_CP_os * sizeof(cf_t));
}

void single_symbol_tx_ofdm_zero_copy(ofdm_t& q,
                                     const cf_t* in,
                                     cf_t* out,
                                     const uint32_t N_b_CP_os) {
    // ifft
    srsran_dft_run_c_zerocopy(&q.plan_tx, in, &out[N_b_CP_os]);

    // cp can be longer than the DFT
    uint32_t cnt = N_b_CP_os;
    while (cnt > q.N_b_DFT_os) {
        cnt -= q.N_b_DFT_os;

        memcpy(&out[cnt], &out[q.N_b_DFT_os], N_b_CP_os * sizeof(cf_t));
    }

    // residual cp
    memcpy(out, &out[q.N_b_DFT_os], N_b_CP_os * sizeof(cf_t));
}

void single_symbol_rx_ofdm(ofdm_t& q, const cf_t* in, cf_t* out, const uint32_t N_b_CP_os) {
    // skip cp and fft
    srsran_dft_run_c(&q.plan_rx, &in[N_b_CP_os], out);
}

void single_symbol_rx_ofdm_zero_copy(ofdm_t& q,
                                     const cf_t* in,
                                     cf_t* out,
                                     const uint32_t N_b_CP_os) {
    // skip cp and fft
    srsran_dft_run_c_zerocopy(&q.plan_rx, &in[N_b_CP_os], out);
}

void mem_mirror(cf_t* in, const uint32_t len) {
    dectnrp_assert(len % 2 == 0, "Mirror length must be a multiple of 2.");

#ifdef OFDM_MEM_MIRROR_ELEMENTWISE
    for (size_t i = 0; i < len / 2; ++i) {
        const cf_t tmp = in[i];
        in[i] = in[i + len / 2];
        in[i + len / 2] = tmp;
    }
#else
    std::vector<cf_t> in_lower(len / 2);
    srsran_vec_cf_copy(in_lower.data(), in, len / 2);
    srsran_vec_cf_copy(in, &in[len / 2], len / 2);
    srsran_vec_cf_copy(&in[len / 2], in_lower.data(), len / 2);
#endif
}

}  // namespace dectnrp::phy::dft
