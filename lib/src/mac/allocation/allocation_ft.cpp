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

#include "dectnrp/mac/allocation/allocation_ft.hpp"

#include <limits>

namespace dectnrp::mac::allocation {

allocation_ft_t::allocation_ft_t(const sp3::duration_lut_t* duration_lut_,
                                 const sp3::duration_t beacon_period_,
                                 const sp3::duration_t beacon_prepare_duration_)
    : allocation_t(duration_lut_, beacon_period_),
      beacon_prepare_duration(beacon_prepare_duration_),
      beacon_time_transmitted_64(std::numeric_limits<int64_t>::max()),
      beacon_time_scheduled_64(std::numeric_limits<int64_t>::max()),
      N_beacons_per_second(duration_lut->get_N_duration_in_second(beacon_period)) {}

}  // namespace dectnrp::mac::allocation
