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

#include "dectnrp/mac/allocation/allocation_pt.hpp"

#include <algorithm>
#include <limits>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"

namespace dectnrp::mac::allocation {

allocation_pt_t::allocation_pt_t(const sp3::duration_lut_t* duration_lut_,
                                 const sp3::duration_t beacon_period_,
                                 const sp3::duration_t allocation_validity_after_beacon_,
                                 const sp3::duration_t allocation_validity_after_now_,
                                 const int64_t turnaround_time_)
    : allocation_t(duration_lut_, beacon_period_),
      allocation_validity_after_beacon(allocation_validity_after_beacon_),
      allocation_validity_after_now(allocation_validity_after_now_),
      turnaround_time(turnaround_time_),
      beacon_time_last_known_64(common::adt::UNDEFINED_EARLY_64) {
    resource_ul_vec.reserve(limits::max_nof_ul_resources_per_pt);
    resource_dl_vec.reserve(limits::max_nof_dl_resources_per_pt);
}

void allocation_pt_t::add_resource(const direction_t direction,
                                   const sp3::duration_t offset,
                                   const sp3::duration_t length) {
    const auto resource = resource_t(offset, length);

    dectnrp_assert(is_within_beacon_period(resource), "outside of beacon period");
    dectnrp_assert(is_orthogonal(direction, resource), "resource not orthogonal");

    switch (direction) {
        using enum direction_t;
        case ul:
            resource_ul_vec.push_back(resource);

            dectnrp_assert(resource_ul_vec.size() <= limits::max_nof_ul_resources_per_pt,
                           "too many uplink resources");
            break;

        case dl:
            resource_dl_vec.push_back(resource);

            dectnrp_assert(resource_dl_vec.size() <= limits::max_nof_dl_resources_per_pt,
                           "too many downlink resources");
            break;

        case both:
            dectnrp_assert_failure("resource can only be added in one direction");
    }
}

void allocation_pt_t::add_resource_regular(const direction_t direction,
                                           const uint32_t offset,
                                           const uint32_t length,
                                           const uint32_t stride,
                                           const uint32_t N,
                                           const sp3::duration_ec_t base_duration_ec) {
    for (uint32_t i = 0; i < N; ++i) {
        add_resource(direction,
                     duration_lut->get_duration(base_duration_ec, offset + stride * i),
                     duration_lut->get_duration(base_duration_ec, length));
    }
}

bool allocation_pt_t::is_within_beacon_period(const resource_t& other) const {
    return other.get_last_sample_index() < beacon_period.get_N_samples();
}

bool allocation_pt_t::is_orthogonal(const resource_vec_t& ref, const resource_t& other) {
    return std::all_of(
        ref.begin(), ref.end(), [other](const auto& elem) { return elem.is_orthogonal(other); });
}

bool allocation_pt_t::is_orthogonal(const direction_t direction, const resource_t& other) const {
    switch (direction) {
        using enum direction_t;
        case ul:
            if (resource_ul_vec.size() == 0) {
                return true;
            }

            return is_orthogonal(resource_ul_vec, other);

        case dl:
            if (resource_dl_vec.size() == 0) {
                return true;
            }

            return is_orthogonal(resource_dl_vec, other);

        case both:
            dectnrp_assert_failure("resource can only be added in one direction");
    }

    dectnrp_assert_failure("unreachable");

    return false;
}

bool allocation_pt_t::is_orthogonal(const direction_t direction,
                                    const resource_vec_t& others) const {
    if (others.size() == 0) {
        return true;
    }

    return std::all_of(others.begin(), others.end(), [&](const auto& elem) {
        return is_orthogonal(direction, elem);
    });
}

tx_opportunity_t allocation_pt_t::get_tx_opportunity(const direction_t direction,
                                                     const int64_t now_64,
                                                     const int64_t tx_earliest_64) const {
    // minimum transmission time
    const int64_t tx_earliest_local_64 = std::max(now_64 + turnaround_time, tx_earliest_64);

    switch (direction) {
        using enum direction_t;
        case ul:
            {
                dectnrp_assert(beacon_time_last_known_64 <= now_64, "uplink times out-of-order");

                // maximum transmission time
                const int64_t tx_latest_local_64 =
                    std::min(beacon_time_last_known_64 +
                                 allocation_validity_after_beacon.get_N_samples<int64_t>(),
                             now_64 + allocation_validity_after_now.get_N_samples<int64_t>());

                return get_tx_opportunity_generic(
                    tx_earliest_local_64, tx_latest_local_64, resource_ul_vec);
            }

        case dl:
            {
                // maximum transmission time
                const int64_t tx_latest_local_64 =
                    beacon_time_last_known_64 + beacon_period.get_N_samples<int64_t>();

                return get_tx_opportunity_generic(
                    tx_earliest_local_64, tx_latest_local_64, resource_dl_vec);
            }

        case both:
            dectnrp_assert_failure("resource can only be added in one direction");
    }

    dectnrp_assert_failure("unreachable");

    return tx_opportunity_t();
}

int64_t allocation_pt_t::get_tx_opportunity_ul_time_closest(const int64_t reference_time_64) const {
    // if no beacon has been sent yet
    if (beacon_time_last_known_64 == common::adt::UNDEFINED_EARLY_64) {
        return common::adt::UNDEFINED_EARLY_64;
    }

    dectnrp_assert(
        beacon_time_last_known_64 < reference_time_64 &&
            reference_time_64 < beacon_time_last_known_64 + beacon_period.get_N_samples<int64_t>(),
        "must lie between two beacons");

    int64_t ret = common::adt::UNDEFINED_EARLY_64;

    for (const auto& elem : resource_ul_vec) {
        const int64_t A =
            reference_time_64 - (beacon_time_last_known_64 + elem.offset.get_N_samples<int64_t>());
        if (std::abs(A) < std::abs(ret)) {
            ret = A;
        }
    }

    return ret;
}

void allocation_pt_t::translate(sp4::resource_allocation_ie_t& raie,
                                const direction_t dir,
                                const uint32_t srdid) const {
    if (dir == direction_t::dl || dir == direction_t::both) {
        raie.allocation_dl = sp4::resource_allocation_ie_t::allocation_t{
            .start_subslot = 1,
            .length_type = sp4::resource_allocation_ie_t::length_type_t::length_in_subslots,
            .length = 2};
    }

    if (dir == direction_t::ul || dir == direction_t::both) {
        raie.allocation_ul = sp4::resource_allocation_ie_t::allocation_t{
            .start_subslot = 1,
            .length_type = sp4::resource_allocation_ie_t::length_type_t::length_in_subslots,
            .length = 2};
    }

    raie.is_additional_allocation = false;

    raie.short_rd_id = srdid;

    raie.repeat_info = sp4::resource_allocation_ie_t::repeat_info_t{
        .repeat_type = sp4::resource_allocation_ie_t::repeat_type_t::repeated_in_following_frames,
        .allow_specific_repeated_resources = false,
        .repetition = 1,
        .validity = 2};

    raie.sfn_offset.reset();

    raie.channel.reset();

    raie.dect_scheduled_resource_failure.reset();
}

tx_opportunity_t allocation_pt_t::get_tx_opportunity_generic(const int64_t tx_earliest_local_64,
                                                             const int64_t tx_latest_local_64,
                                                             const resource_vec_t& rvec) const {
    // we can have no allocated resources
    if (rvec.empty()) {
        return tx_opportunity_t();
    };

    /* The following condition can be true if we get data from the upper layers and start
     * searching for a TX opportunity, but haven't received any beacons at all or at least for a
     * very long time.
     */
    while (tx_earliest_local_64 > tx_latest_local_64) {
        return tx_opportunity_t();
    }

    // clang-format off
    /*                          beacon_period_64 <-------->
     *... _____|__________|__________|__________|__________|__________|_______ ...
     *                                                     |<---beacon_time_last_known_64
     *                                                     |<---A
     *                                                |<---tx_earliest_local_64
     */
    // clang-format on

    // clang-format off
    /*
     *... _____|__________|__________|__________|__________|__________|_______ ...
     *                    |<-----------B------------->|
     *                    |<----------C-------->|     |
     *                    |                A--->|     |
     *                    |                           |<---tx_earliest_local_64
     *                    |
     *                    |<---beacon_time_last_known_64
     */
    // clang-format on

    // upper drawing
    int64_t A = beacon_time_last_known_64;

    // lower drawing
    if (tx_earliest_local_64 > beacon_time_last_known_64) {
        const int64_t B = tx_earliest_local_64 - beacon_time_last_known_64;
        const int64_t C = common::adt::multiple_leq(B, beacon_period.get_N_samples<int64_t>());

        A = beacon_time_last_known_64 + C;

        dectnrp_assert(A <= tx_earliest_local_64, "next multiple incorrect");
    }

    // we need to find a transmission time and the number of samples we can transmit
    int64_t tx_time_64 = std::numeric_limits<int64_t>::max();
    int64_t N_samples_64 = 0;

    // the allocation repeats from frame to frame, so we can test the same resource multiple times
    uint32_t idx = 0;

    // keep searching until ...
    while (1) {
        // potential transmission time
        const int64_t D = A + rvec.at(idx).offset.get_N_samples<int64_t>();

        // ... we either found the first possible transmission time within the limits or ...
        if (tx_earliest_local_64 <= D && D < tx_latest_local_64) {
            tx_time_64 = D;
            N_samples_64 = rvec.at(idx).length.get_N_samples<int64_t>();
            break;
        }

        // ... until we are outside of the limits
        if (tx_latest_local_64 <= D) {
            return tx_opportunity_t();
        }

        // wrap and continue search
        if (++idx == rvec.size()) {
            // wrap
            idx = 0;

            // increase base time by one beacon period
            A += beacon_period.get_N_samples<int64_t>();
        }
    }

    dectnrp_assert(tx_earliest_local_64 <= tx_time_64, "tx time too early");
    dectnrp_assert(tx_time_64 < tx_latest_local_64, "tx time too late");

    return tx_opportunity_t(tx_time_64, N_samples_64);
}

}  // namespace dectnrp::mac::allocation
