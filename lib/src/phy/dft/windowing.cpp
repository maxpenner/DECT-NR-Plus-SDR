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

#include "dectnrp/phy/dft/windowing.hpp"

#include <algorithm>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/filter/raised_cosine.hpp"

namespace dectnrp::phy::dft {

void get_windowing(windowing_t& q, const uint32_t N_CP_length, const float fraction) {
    // determine length of window
    const uint32_t N =
        static_cast<uint32_t>(std::round(static_cast<float>(N_CP_length) * fraction));

    dectnrp_assert(2 <= N, "window length must be at least 2 samples");

    // load filter coefficients
    const std::vector<float> rc = filter::raised_cosine_window_rising_edge(N);

    // mirrored version
    std::vector<float> rc_inv = rc;
    std::reverse(std::begin(rc_inv), std::end(rc_inv));

    // allocate aligned memory ...
    q.raised_cosine = srsran_vec_f_malloc(N);
    q.raised_cosine_inv = srsran_vec_f_malloc(N);
    q.length = N;

    // ... and copy to it
    srsran_vec_f_copy(q.raised_cosine, &rc[0], N);
    srsran_vec_f_copy(q.raised_cosine_inv, &rc_inv[0], N);
}

void free_windowing(windowing_t& q) {
    free(q.raised_cosine);
    free(q.raised_cosine_inv);
}

}  // namespace dectnrp::phy::dft
