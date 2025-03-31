/**
 * Copyright 2013-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "dectnrp/simulation/wireless/pathloss.hpp"

#include <cmath>

namespace dectnrp::simulation::pathloss {

float fspl(const float d_m, const float f_Hz) {
    // interpret null input as a request to return null output
    if (d_m <= 0.0f || f_Hz <= 0.0f) {
        return 0.0f;
    }

    double fspl_val = 20.0 * std::log10(double(d_m)) + 20.0f * std::log10(double(f_Hz)) - 147.55;

    // like Matlab, we set the minimum value to 0
    return (fspl_val < 0.0) ? 0.0f : float(fspl_val);
}

}  // namespace dectnrp::simulation::pathloss
