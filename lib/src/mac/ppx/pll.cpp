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

#include "dectnrp/mac/ppx/pll.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/mac/ppx/pll_pps_param.hpp"

namespace dectnrp::mac::ppx {

pll_t::pll_t(const int64_t ppx_period_64_)
    : ppx_period_64(ppx_period_64_) {
    dectnrp_assert(0 < PPX_PPS_PLL_PARAM_NUMBER_OF_PPS_FOR_ESTIMATION, "ill-defined");
    dectnrp_assert(PPX_PPS_PLL_PARAM_NUMBER_OF_PPS_FOR_ESTIMATION <= 60, "ill-defined");

    ppx_rising_edge_vec.resize(PPX_PPS_PLL_PARAM_NUMBER_OF_PPS_FOR_ESTIMATION);
};

void pll_t::provide_measured_ppx_rising_edge(const int64_t ppx_rising_edge_64) {
    ppx_rising_edge_vec.at(idx) = ppx_rising_edge_64;
    ++idx;
    idx %= ppx_rising_edge_vec.size();
}

int64_t pll_t::warp_duration(const int64_t duration_64) const {
    const std::size_t oldest = idx == (ppx_rising_edge_vec.size() - 1) ? 0 : idx + 1;

    // do we have the corresponding oldest value?
    if (ppx_rising_edge_vec[oldest] < 0) {
        return duration_64;
    }

    const int64_t range_64 = ppx_rising_edge_vec[idx] - ppx_rising_edge_vec[oldest];

    const int64_t likely_number_of_ppx_64 = common::adt::round_integer(range_64, ppx_period_64);

    const int64_t samp_rate_warped_64 = range_64 / likely_number_of_ppx_64;

    return duration_64 * samp_rate_warped_64 / ppx_period_64;
}

}  // namespace dectnrp::mac::ppx
