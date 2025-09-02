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

#include "dectnrp/simulation/vspp/vspprx.hpp"

#include <cstring>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::simulation {

vspprx_t::vspprx_t(const uint32_t id_,
                   const uint32_t nof_antennas_,
                   const uint32_t samp_rate_,
                   const uint32_t spp_size_)
    : vspp_t(id_, nof_antennas_, samp_rate_, spp_size_) {
    meta.set_reasonable_default_values(nof_antennas);
}

void vspprx_t::spp_read(std::vector<cf_t*>& out, uint32_t nof_samples) const {
    dectnrp_assert(are_args_valid(out.size(), 0, nof_samples), "Output ill-configured");

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas; ++ant_idx) {
        srsran_vec_cf_copy(out[ant_idx], spp[ant_idx], nof_samples);
    }
}

void vspprx_t::spp_read_v(std::vector<void*>& out, uint32_t nof_samples) const {
    dectnrp_assert(are_args_valid(out.size(), 0, nof_samples), "Output ill-configured");

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas; ++ant_idx) {
        srsran_vec_cf_copy((cf_t*)out[ant_idx], spp[ant_idx], nof_samples);
    }
}

void vspprx_t::meta_t::set_reasonable_default_values(const uint32_t nof_antennas) {
    now_64 = common::adt::UNDEFINED_EARLY_64;

    rx_power_ant_0dBFS = common::ant_t(nof_antennas);
    rx_power_ant_0dBFS.fill(0.0f);
    rx_noise_figure_dB = 5.0f;
    rx_snr_in_net_bandwidth_norm_dB = 40.0f;
}

}  // namespace dectnrp::simulation
