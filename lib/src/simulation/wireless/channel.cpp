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

#include "dectnrp/simulation/wireless/channel.hpp"

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/simulation/wireless/pathloss.hpp"

namespace dectnrp::simulation {

channel_t::channel_t(const uint32_t id_0_,
                     const uint32_t id_1_,
                     const uint32_t samp_rate_,
                     const uint32_t spp_size_)
    : id_0(id_0_),
      id_1(id_1_),
      samp_rate(samp_rate_),
      spp_size(spp_size_) {
    large_scale_stage = srsran_vec_cf_malloc(spp_size);
    small_scale_stage = srsran_vec_cf_malloc(spp_size);
}

channel_t::~channel_t() {
    free(large_scale_stage);
    free(small_scale_stage);
}

bool channel_t::check_args(const vspptx_t& vspptx, const vspprx_t& vspprx) const {
    return (vspptx.id == id_0 && vspprx.id == id_1) || (vspptx.id == id_1 && vspprx.id == id_0);
}

float channel_t::get_large_scale_via_pathloss(const vspptx_t& vspptx,
                                              const vspprx_t& vspprx,
                                              const vspptx_t& vspptx_other,
                                              const size_t tx_idx,
                                              const size_t rx_idx) {
    // there are two ways to define large scale fading
    float large_scale_fading_dB = 0.0f;

    // either the nodes are different, then it is defined through the distance in space ...
    if (vspptx.id != vspptx_other.id) {
        const float distance = vspptx_other.meta.position.distance(vspptx.meta.position);
        large_scale_fading_dB = pathloss::fspl(distance, vspptx_other.meta.freq_Hz);
    }
    // ... or it is the same node, then we assume the self-leakage to be the large scale fading
    else {
        large_scale_fading_dB = vspptx.meta.tx_into_rx_leakage_dB;
    }

    return common::adt::db2mag(vspptx_other.meta.tx_power_ant_0dBFS - large_scale_fading_dB -
                               vspprx.meta.rx_power_ant_0dBFS.at(rx_idx));
}

}  // namespace dectnrp::simulation
