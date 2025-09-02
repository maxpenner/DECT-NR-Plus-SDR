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

#include "dectnrp/sections_part3/derivative/duration.hpp"
#include "dectnrp/sections_part3/derivative/duration_lut.hpp"

namespace dectnrp::mac::allocation {

class allocation_t {
    public:
        const sp3::duration_t& get_beacon_period_as_duration() const { return beacon_period; };
        int64_t get_beacon_period() const { return beacon_period.get_N_samples<int64_t>(); };

    protected:
        /// abstract class
        allocation_t() = default;

        /// abstract class
        explicit allocation_t(const sp3::duration_lut_t* duration_lut_,
                              const sp3::duration_t beacon_period_)
            : duration_lut(duration_lut_),
              beacon_period(beacon_period_) {};

        const sp3::duration_lut_t* duration_lut{nullptr};

        /// typical values are 10ms, 20ms, 50ms etc.
        sp3::duration_t beacon_period;
};

}  // namespace dectnrp::mac::allocation
