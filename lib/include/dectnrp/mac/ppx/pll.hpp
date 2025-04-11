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

#pragma once

#include <cstdint>
#include <vector>

#include "dectnrp/common/adt/miscellaneous.hpp"

namespace dectnrp::mac::ppx {

class pll_t {
    public:
        pll_t(const int64_t ppx_period_64_);

        void provide_measured_ppx_rising_edge(const int64_t ppx_rising_edge_64);

        int64_t warp_duration(const int64_t duration_64) const;

    private:
        int64_t ppx_period_64{common::adt::UNDEFINED_EARLY_64};

        std::vector<int64_t> ppx_rising_edge_vec;
        std::size_t idx{};
};

}  // namespace dectnrp::mac::ppx
