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

#include "dectnrp/phy/rx/localbuffer.hpp"

#include <limits>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

localbuffer_t::localbuffer_t(const uint32_t nof_antennas_limited_, const uint32_t nof_samples_)
    : nof_antennas_limited(nof_antennas_limited_),
      nof_samples(nof_samples_),
      ant_streams_time_64(std::numeric_limits<int64_t>::max()),
      cnt_w(0) {
    for (uint32_t i = 0; i < nof_antennas_limited; ++i) {
        buffer_vec.push_back(srsran_vec_cf_malloc(nof_samples));
    }
}

localbuffer_t::~localbuffer_t() {
    for (auto elem : buffer_vec) {
        free(elem);
    }
}

}  // namespace dectnrp::phy
