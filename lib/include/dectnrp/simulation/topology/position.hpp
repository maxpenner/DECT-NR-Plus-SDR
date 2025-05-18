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

namespace dectnrp::simulation::topology {

class position_t {
    public:
        position_t() = default;

        static position_t from_cartesian(const float x_, const float y_, const float z_) {
            return position_t(x_, y_, z_);
        }

        static position_t from_polar(const float radius, const float angle_deg) {
            return position_t(radius, angle_deg);
        }

        position_t& operator+=(const position_t& position) {
            x += position.x;
            y += position.y;
            z += position.z;
            return *this;
        }

        position_t& operator*=(const float fac) {
            x *= fac;
            y *= fac;
            z *= fac;
            return *this;
        }

        float distance(const position_t& other) const;

        friend class trajectory_t;

    private:
        position_t(const float x_, const float y_, const float z_)
            : x(x_),
              y(y_),
              z(z_) {}

        position_t(const float radius, const float angle_deg);

        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
};

}  // namespace dectnrp::simulation::topology
