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

#pragma once

#include <cstdint>

#include "dectnrp/simulation/topology/position.hpp"

namespace dectnrp::simulation::topology {

class trajectory_t {
    public:
        trajectory_t() = default;

        /// point with no movement
        trajectory_t(const position_t offset_)
            : trajectory_shape(trajectory_shape_t::point),
              offset(offset_) {};

        /// circle
        trajectory_t(const position_t offset_, const float speed_, const float radius_);

        /// line segment
        trajectory_t(const position_t offset_,
                     const float speed_,
                     const float line_length_,
                     const float line_angle_rad_);

        void update_position(position_t& position,
                             const uint32_t samp_rate,
                             const uint64_t now_64) const;

    private:
        enum class trajectory_shape_t {
            point,
            circle,
            line_segment
        };

        trajectory_shape_t trajectory_shape{trajectory_shape_t::point};
        position_t offset;
        float direction{0.0f};

        float radius{0.0f};

        float line_length{0.0f};
        float line_angle_rad{0.0f};

        float period_sec{0.0f};
};

}  // namespace dectnrp::simulation::topology
