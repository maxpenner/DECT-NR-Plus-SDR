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

#include "dectnrp/simulation/topology/position.hpp"

#include <cmath>
#include <numbers>

namespace dectnrp::simulation::topology {

position_t::position_t(const float radius, const float angle_deg)
    : x(radius * std::cos(2.0f * std::numbers::pi_v<float> * angle_deg / 360.0f)),
      y(radius * std::sin(2.0f * std::numbers::pi_v<float> * angle_deg / 360.0f)) {}

float position_t::distance(const position_t& other) const {
    return std::sqrt(std::pow(x - other.x, 2.0f) + std::pow(y - other.y, 2.0f) +
                     std::pow(z - other.z, 2.0f));
}

}  // namespace dectnrp::simulation::topology
