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

#include "dectnrp/simulation/hardware/quantize.hpp"

#include <cmath>

namespace dectnrp::simulation {

void quantize_re_im(const std::vector<cf_t*> inp,
                    std::vector<cf_t*> out,
                    const uint32_t nof_samples,
                    const float bit_width) {
    for (uint32_t ant_idx = 0; ant_idx < inp.size(); ++ant_idx) {
        for (uint32_t i = 0; i < nof_samples; ++i) {
            // split into real and imaginary
            float re = __real__(inp[ant_idx][i]);
            float im = __imag__(inp[ant_idx][i]);

            re = std::roundf(re / bit_width);
            im = std::roundf(im / bit_width);

            out[ant_idx][i] = cf_t{re * bit_width, im * bit_width};
        }
    }
}

}  // namespace dectnrp::simulation
