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

#include "dectnrp/simulation/vspp/vspptx.hpp"

#include <cstring>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::simulation {

vspptx_t::vspptx_t(const uint32_t id_,
                   const uint32_t nof_antennas_,
                   const uint32_t samp_rate_,
                   const uint32_t spp_size_)
    : vspp_t(id_, nof_antennas_, samp_rate_, spp_size_) {
    meta.set_reasonable_default_values(nof_antennas);
}

void vspptx_t::spp_write(const std::vector<cf_t*>& inp, uint32_t offset, uint32_t nof_samples) {
    dectnrp_assert(are_args_valid(inp.size(), offset, nof_samples), "Input ill-configured");

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas; ++ant_idx) {
        srsran_vec_cf_copy(&spp[ant_idx][offset], inp[ant_idx], nof_samples);
    }
}

void vspptx_t::spp_write_v(const std::vector<void*>& inp, uint32_t offset, uint32_t nof_samples) {
    dectnrp_assert(are_args_valid(inp.size(), offset, nof_samples), "Input ill-configured");

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas; ++ant_idx) {
        srsran_vec_cf_copy(&spp[ant_idx][offset], (cf_t*)inp[ant_idx], nof_samples);
    }
}

void vspptx_t::deepcopy(vspptx_t& dst) const {
    dst.spp_write(spp, 0, spp_size);

    dst.tx_idx = tx_idx;
    dst.tx_length = tx_length;

    memcpy(&dst.meta, &meta, sizeof(vspptx_t::meta_t));
}

void vspptx_t::tx_set_no_non_zero_samples() {
    tx_idx = -1;
    tx_length = -1;
}

void vspptx_t::meta_t::set_reasonable_default_values(const uint32_t nof_antennas) {
    vspprx_counterpart_registered = false;

    now_64 = common::adt::UNDEFINED_EARLY_64;

    freq_Hz = 1.0e9f;
    net_bandwidth_norm = 1.0f;

    tx_power_ant_0dBFS = common::ant_t(nof_antennas);
    tx_into_rx_leakage_dB = 1000.0f;
}

}  // namespace dectnrp::simulation
