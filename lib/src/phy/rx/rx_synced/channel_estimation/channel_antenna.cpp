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

#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_antenna.hpp"

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/sections_part3/drs.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::phy {

channel_antenna_t::channel_antenna_t(const uint32_t b_max_, const uint32_t N_eff_TX_max_) {
    // load physical dimensions
    const uint32_t b_idx_max = sp3::phyres::b2b_idx[b_max_];
    const uint32_t N_b_OCC_plus_DC_max = sp3::phyres::N_b_OCC_plus_DC_lut[b_idx_max];

    // maixmum number of DRS cells per OFDM symbol and transmit stream
    const uint32_t nof_drs_subc_max = sp3::drs_t::get_nof_drs_subc(b_max_);

    // init for every TS
    for (uint32_t ts_idx = 0; ts_idx < N_eff_TX_max_; ++ts_idx) {
        chestim_drs_zf.push_back(srsran_vec_cf_malloc(nof_drs_subc_max));
        chestim_drs_zf_interlaced.push_back(srsran_vec_cf_malloc(nof_drs_subc_max * 2));
        chestim.push_back(srsran_vec_cf_malloc(N_b_OCC_plus_DC_max));
    }
}

channel_antenna_t::~channel_antenna_t() {
    for (auto& elem : chestim_drs_zf) {
        free(elem);
    }

    for (auto& elem : chestim_drs_zf_interlaced) {
        free(elem);
    }

    for (auto& elem : chestim) {
        free(elem);
    }
}

}  // namespace dectnrp::phy
