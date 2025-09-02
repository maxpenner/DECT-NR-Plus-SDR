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

#include "dectnrp/phy/indicators/cqi_lut.hpp"

#include <algorithm>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::indicators {

cqi_lut_t::cqi_lut_t(const uint32_t mcs_min_, const uint32_t mcs_max_, const float snr_offset_)
    : mcs_min(mcs_min_),
      mcs_max(mcs_max_),
      snr_offset(snr_offset_) {
    dectnrp_assert(mcs_min <= mcs_max, "MCS not ordered");
    dectnrp_assert(mcs_max_ < snr_required.size(), "MCS max too large");
    dectnrp_assert(0 <= snr_offset, "snr_offset should be >= 0 for pessimism when choosing MCS");
}

uint32_t cqi_lut_t::get_highest_mcs_possible(const float snr_dB_measured) const {
    // assume we have received a lower SNR than we actually did
    const float snr_pessimistic = snr_dB_measured - snr_offset;

    uint32_t ret = mcs_min;

    // find highest MCS that is possible at snr_pessimistic
    for (uint32_t i = mcs_min + 1; i <= mcs_max; ++i) {
        if (snr_required[i] <= snr_pessimistic) {
            ret = i;
        } else {
            break;
        }
    }

    return ret;
}

uint32_t cqi_lut_t::clamp_mcs(const uint32_t mcs_candidate) const {
    return std::clamp(mcs_candidate, mcs_min, mcs_max);
}

}  // namespace dectnrp::phy::indicators
