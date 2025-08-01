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

#include <vector>

namespace dectnrp::radio {

class antenna_array_t {
    public:
        enum class arrangement_t {
            linear,
            linear_uneven,
            circle
        };

        antenna_array_t() = default;

        explicit antenna_array_t(const arrangement_t arrangement_, const float spacing_);

        /// constructor for uneven spacing
        explicit antenna_array_t(const arrangement_t arrangement_,
                                 const std::vector<float> spacing_);

    private:
        arrangement_t arrangement{arrangement_t::linear};

        /// antenna spacing in meters
        std::vector<float> spacing{0.05};
};

}  // namespace dectnrp::radio
