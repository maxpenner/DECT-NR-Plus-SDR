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

#include <cstdint>
#include <vector>

#include "dectnrp/mac/allocation/allocation.hpp"
#include "dectnrp/mac/allocation/resource.hpp"
#include "dectnrp/mac/allocation/tx_opportunity.hpp"

namespace dectnrp::mac::allocation {

class allocation_pt_t final : public allocation_t {
    public:
        allocation_pt_t() = default;

        explicit allocation_pt_t(const section3::duration_lut_t* duration_lut_,
                                 const section3::duration_t beacon_period_,
                                 const section3::duration_t allocation_validity_after_beacon_,
                                 const section3::duration_t allocation_validity_after_now_,
                                 const int64_t turnaround_time_);

        typedef std::vector<resource_t> resource_vec_t;

        int64_t get_beacon_time_last_known() const { return beacon_time_last_known_64; };
        void set_beacon_time_last_known(const int64_t beacon_time_last_known_64_) {
            beacon_time_last_known_64 = beacon_time_last_known_64_;
        };

        void add_resource(const direction_t direction,
                          const section3::duration_t offset,
                          const section3::duration_t length);

        void add_resource_regular(const direction_t direction,
                                  const uint32_t offset,
                                  const uint32_t length,
                                  const uint32_t stride,
                                  const uint32_t N,
                                  const section3::duration_ec_t base_duration_ec);

        bool is_within_beacon_period(const resource_t& other) const;

        static bool is_orthogonal(const resource_vec_t& ref, const resource_t& other);

        bool is_orthogonal(const direction_t direction, const resource_t& other) const;

        bool is_orthogonal(const direction_t direction, const resource_vec_t& others) const;

        /// FTs request downlink resources of the PTs, PTs request uplink resources
        tx_opportunity_t get_tx_opportunity(const direction_t direction,
                                            const int64_t now_64,
                                            const int64_t tx_earliest_64) const;

        /// after receiving an uplink, we can request the closest tx_opportunity
        int64_t get_tx_opportunity_ul_time_closest(const int64_t reference_time_64) const;

    private:
        /// duration after receiving a beacon in which we are allowed to transmit
        section3::duration_t allocation_validity_after_beacon;

        /// duration after current time in which we are allowed to transmit
        section3::duration_t allocation_validity_after_now;

        /// common SDR terminology
        int64_t turnaround_time;

        /// start time of the last beacon we heard from our FT
        int64_t beacon_time_last_known_64;

        resource_vec_t resource_ul_vec;
        resource_vec_t resource_dl_vec;

        tx_opportunity_t get_tx_opportunity_generic(const int64_t tx_earliest_local_64,
                                                    const int64_t tx_latest_local_64,
                                                    const resource_vec_t& rvec) const;
};

}  // namespace dectnrp::mac::allocation
