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

#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_statistics.hpp"

#include <cmath>

#include "dectnrp/phy/filter/bessel.hpp"
#include "dectnrp/phy/filter/rectangular.hpp"

namespace dectnrp::phy {

float channel_statistics_t::get_r_f_uni(const float tau_rms_sec, const float delta_f) {
    return filter::sinc(float(M_PI) * tau_rms_sec * delta_f);
}

float channel_statistics_t::get_r_t_jakes(const float nu_max_hz, const float delta_t) {
    return filter::bessel_function_J_0(2.0f * float(M_PI) * nu_max_hz * delta_t);
}

}  // namespace dectnrp::phy
