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

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/mac/allocation/allocation.hpp"

namespace dectnrp::mac::allocation {

class allocation_ft_t final : public allocation_t {
    public:
        allocation_ft_t() = default;

        explicit allocation_ft_t(const sp3::duration_lut_t* duration_lut_,
                                 const sp3::duration_t beacon_period_,
                                 const sp3::duration_t beacon_prepare_duration_);

        int64_t get_beacon_time_transmitted() const { return beacon_time_transmitted_64; };

        int64_t get_beacon_time_scheduled() const { return beacon_time_scheduled_64; };

        /// check if beacon has to be prepared for transmission
        int64_t get_beacon_time_scheduled_minus_prepare_duration() const {
            return beacon_time_scheduled_64 - beacon_prepare_duration.get_N_samples<int64_t>();
        }

        /// called to schedule first beacon
        void set_beacon_time_scheduled(const int64_t beacon_time_scheduled_64_) {
            beacon_time_scheduled_64 = beacon_time_scheduled_64_;
        }

        uint64_t get_beacon_cnt() const { return beacon_cnt; };

        /// once a beacon was transmitted successfully, this function updates internal times
        void set_beacon_time_next() {
            // beacon was transmitted as scheduled
            beacon_time_transmitted_64 = beacon_time_scheduled_64;
            beacon_time_scheduled_64 += beacon_period.get_N_samples<int64_t>();
            ++beacon_cnt;
        }

        uint32_t get_N_beacons_per_second() const { return N_beacons_per_second; };

    private:
        sp3::duration_t beacon_prepare_duration{};

        /// start time of last beacon transmitted
        int64_t beacon_time_transmitted_64{common::adt::UNDEFINED_EARLY_64};

        /// start time of next scheduled beacon
        int64_t beacon_time_scheduled_64{common::adt::UNDEFINED_EARLY_64};

        /// transmitted beacons
        uint64_t beacon_cnt{};

        /// number of beacons transmitted per second
        uint32_t N_beacons_per_second{};
};

}  // namespace dectnrp::mac::allocation
