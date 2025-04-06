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

#include "dectnrp/phy/rx/rx_synced/estimator/estimator.hpp"

#include <algorithm>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/sections_part3/drs.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::phy {

void estimator_t::reset(const uint32_t b, const uint32_t N_eff_TX_) {
    // convert beta to its index
    const uint32_t b_idx = section3::phyres::b2b_idx[b];

    // derive number of subcarrier from b and b_idx
    N_b_OCC_plus_DC = section3::phyres::N_b_OCC_plus_DC_lut[b_idx];
    N_STF_cells_b = b * constants::N_STF_cells_b_1;
    N_DRS_cells_b = section3::drs_t::get_nof_drs_subc(b);

    N_eff_TX = N_eff_TX_;

    dectnrp_assert(N_STF_cells_b == N_DRS_cells_b, "number of STF and DRS must be the same");

    // every estimator must implement its private reset function
    reset_internal();
}

}  // namespace dectnrp::phy
