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

#include "dectnrp/simulation/topology/trajectory.hpp"

#include <cmath>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::simulation::topology {

trajectory_t::trajectory_t(const position_t offset_, const float speed_, const float radius_)
    : trajectory_shape(trajectory_shape_t::circle),
      offset(offset_),
      direction(0.0f < speed_ ? 1.0f : -1.0f),
      radius(radius_) {
    // absolute speed
    const float speed_abs = std::abs(speed_);

    dectnrp_assert(
        0.01f <= speed_abs && speed_abs < 100.0f, "on circles we must move {}", speed_abs);

    dectnrp_assert(1.0f <= radius && radius <= 10.0e3, "circle dimensions limited {}", radius);

    period_sec = 2.0f * std::numbers::pi_v<double> * double(radius) / double(speed_abs);
};

trajectory_t::trajectory_t(const position_t offset_,
                           const float speed_,
                           const float line_length_,
                           const float line_angle_rad_)
    : trajectory_shape(trajectory_shape_t::line_segment),
      offset(offset_),
      direction(0.0f < speed_ ? 1.0f : -1.0f),
      line_length(line_length_),
      line_angle_rad(line_angle_rad_) {
    // absolute speed
    const float speed_abs = std::abs(speed_);

    dectnrp_assert(0.01f <= speed_abs && speed_abs < 100.0f, "on lines we must move {}", speed_abs);

    dectnrp_assert(
        1.0f <= line_length && line_length <= 10.0e3, "line dimensions limited {}", line_length);

    period_sec = 2.0f * line_length / speed_abs;
};

void trajectory_t::update_position(position_t& position,
                                   const uint32_t samp_rate,
                                   const uint64_t now_64) const {
    // point, so nothing to do here
    if (trajectory_shape == trajectory_shape_t::point) {
        position = offset;
        return;
    }

    // convert period_sec to 64 sample equivalent
    const int64_t period_samples_64 = static_cast<int64_t>(period_sec * float(samp_rate));

    // how far are we in the current period?
    const int64_t phase_in_period_samples_64 = now_64 % period_samples_64;

    // convert to +/- 2*pi
    const float phase_0_to_1 =
        static_cast<float>(phase_in_period_samples_64) / static_cast<float>(period_samples_64);

    switch (trajectory_shape) {
        using enum trajectory_shape_t;

        case circle:
            position.x = radius * std::cos(2.0f * std::numbers::pi_v<float> * phase_0_to_1);
            position.y = radius * std::sin(2.0f * std::numbers::pi_v<float> * phase_0_to_1);
            break;

        case line_segment:
            {
                const float A = (phase_0_to_1 <= 0.5f) ? line_length * phase_0_to_1 / 0.5f
                                                       : line_length * (1.0f - phase_0_to_1) / 0.5f;

                position.x = A * std::cos(line_angle_rad);
                position.y = A * std::sin(line_angle_rad);
            }
            break;

        default:
            dectnrp_assert_failure("unreachable point reached");
    }

    position *= direction;

    position += offset;
}

}  // namespace dectnrp::simulation::topology
