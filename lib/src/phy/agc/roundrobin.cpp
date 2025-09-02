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

#include "dectnrp/phy/agc/roundrobin.hpp"

#include <algorithm>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::agc {

roundrobin_t::roundrobin_t(const uint32_t nof_antennas_, const uint32_t simultaneous_)
    : nof_antennas(nof_antennas_),
      simultaneous(std::min(simultaneous_, nof_antennas_)) {
    dectnrp_assert(0 < simultaneous, "too small");
    dectnrp_assert(simultaneous <= nof_antennas, "too large");
}

common::ant_t roundrobin_t::process(const common::ant_t& ant) {
    common::ant_t ret(nof_antennas);

    for (uint32_t i = 0; i < simultaneous; ++i) {
        if (ant.at(r_idx) != 0.0f) {
            ret.at(r_idx) = ant.at(r_idx);
        }

        ++r_idx;

        if (r_idx == nof_antennas) {
            r_idx = 0;
        }
    }

    return ret;
}

}  // namespace dectnrp::phy::agc
