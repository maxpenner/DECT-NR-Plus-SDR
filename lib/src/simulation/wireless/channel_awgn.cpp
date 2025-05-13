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

#include "dectnrp/simulation/wireless/channel_awgn.hpp"

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::simulation {

const std::string channel_awgn_t::name = "awgn";

channel_awgn_t::channel_awgn_t(const uint32_t id_0_,
                               const uint32_t id_1_,
                               const uint32_t samp_rate_,
                               const uint32_t spp_size_)
    : channel_t(id_0_, id_1_, samp_rate_, spp_size_) {}

void channel_awgn_t::superimpose(const vspptx_t& vspptx,
                                 vspprx_t& vspprx,
                                 const vspptx_t& vspptx_other) const {
    dectnrp_assert(check_args(vspptx_other, vspprx), "Incorrect two nodes");

    // superimpose every TX antenna ..
    for (uint32_t tx_idx = 0; tx_idx < vspptx_other.nof_antennas; ++tx_idx) {
        // ... onto every RX antenna
        for (uint32_t rx_idx = 0; rx_idx < vspprx.nof_antennas; ++rx_idx) {
            // large scale also includes RX sensitivity
            const float large_scale =
                get_large_scale_via_pathloss(vspptx, vspprx, vspptx_other, rx_idx);

            // apply large scale fading
            srsran_vec_sc_prod_cfc(
                vspptx_other.spp[tx_idx], large_scale, large_scale_stage, vspprx.spp_size);

            // apply small scale fading
            // nothing to do here

            // apply superposition
            srsran_vec_sum_ccc(
                large_scale_stage, vspprx.spp[rx_idx], vspprx.spp[rx_idx], vspprx.spp_size);
        }
    }
}

void channel_awgn_t::randomize_small_scale() {
    // nothing to do here
}

}  // namespace dectnrp::simulation
