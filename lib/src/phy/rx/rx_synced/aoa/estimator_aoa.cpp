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

#include "dectnrp/phy/rx/rx_synced/aoa/estimator_aoa.hpp"

namespace dectnrp::phy {

estimator_aoa_t::estimator_aoa_t([[maybe_unused]] const uint32_t b_max, const uint32_t N_RX_)
    : estimator_t(),
      N_RX(N_RX_) {}

estimator_aoa_t::~estimator_aoa_t() {}

void estimator_aoa_t::process_stf([[maybe_unused]] const channel_antennas_t& channel_antennas,
                                  [[maybe_unused]] const process_stf_meta_t& process_stf_meta) {}

void estimator_aoa_t::process_drs([[maybe_unused]] const channel_antennas_t& channel_antennas,
                                  [[maybe_unused]] const process_drs_meta_t& process_drs_meta) {}

void estimator_aoa_t::reset_internal() {}

}  // namespace dectnrp::phy
