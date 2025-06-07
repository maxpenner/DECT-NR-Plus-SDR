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

#include "dectnrp/phy/pool/irregular.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"

namespace dectnrp::phy {

irregular_t::irregular_t() {
    dectnrp_assert(std::all_of(irregular_report_arr.cbegin(),
                               irregular_report_arr.cend(),
                               [](const auto& elem) { return !elem.has_finite_time(); }),
                   "incorrect default value");
}

void irregular_t::push(const irregular_report_t&& irregular_report) {
    dectnrp_assert(irregular_report.has_finite_time(), "invalid");

    lockv.lock();

    // find free slot
    const auto it_next_candidate = std::find_if(
        irregular_report_arr.begin(), irregular_report_arr.end(), [](const auto& elem) {
            return !elem.has_finite_time();
        });

    dectnrp_assert(it_next_candidate < irregular_report_arr.cend(), "no space left");

    // copy
    *it_next_candidate = irregular_report;

    // is it earlier than the current next?
    if (irregular_report.call_asap_after_this_time_has_passed_64 <
        next_time_64.load(std::memory_order_acquire)) {
        it_next = it_next_candidate;
        next_time_64.store(irregular_report.call_asap_after_this_time_has_passed_64,
                           std::memory_order_release);
    }

    lockv.unlock();
}

int64_t irregular_t::get_next_time() const {
    const auto ret = next_time_64.load(std::memory_order_acquire);

    if (ret < irregular_report_t::undefined_late) {
        return ret;
    }

    return irregular_report_t::undefined_late;
};

irregular_report_t irregular_t::pop() {
    lockv.lock();

    dectnrp_assert(it_next != nullptr, "pop value does not exist");
    dectnrp_assert(it_next < irregular_report_arr.cend(), "pop value does not exist");

    // make a copy
    const irregular_report_t ret = *it_next;

    // free slot
    *it_next = irregular_report_t();

    // find next smallest callback time
    auto it = std::min_element(irregular_report_arr.begin(),
                               irregular_report_arr.end(),
                               [](const auto& lhs, const auto& rhs) {
                                   return lhs.call_asap_after_this_time_has_passed_64 <
                                          rhs.call_asap_after_this_time_has_passed_64;
                               });

    if (it->has_finite_time()) {
        it_next = it;
        next_time_64 = it->call_asap_after_this_time_has_passed_64;
    } else {
        it_next = nullptr;
        next_time_64 = irregular_report_t::undefined_late;
    }

    lockv.unlock();

    return ret;
}

}  // namespace dectnrp::phy
