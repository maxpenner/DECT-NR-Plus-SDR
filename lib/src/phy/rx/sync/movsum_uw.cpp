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

#include "dectnrp/phy/rx/sync/movsum_uw.hpp"

#include <algorithm>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

namespace dectnrp::phy {

movsum_uw_t::movsum_uw_t(const std::vector<float> uw_, const uint32_t n_repelem_)
    : movsum_t<lv_32fc_t>(uw_.size() * n_repelem_),
      uw(uw_),
      n_repelem(n_repelem_),
      prefactor_push(uw.back()),
      prefactor_pop(uw.front()),
      uw_pos_neg_idx(get_uw_changes_idx(uw, n_repelem, true)),
      uw_neg_pos_idx(get_uw_changes_idx(uw, n_repelem, false)) {
    dectnrp_assert(uw.size() * n_repelem == length, "length incorrect");
    dectnrp_assert(prefactor_push == 1.0f, "prefactor for push incorrect");
    dectnrp_assert(std::abs(prefactor_pop) == 1.0f, "prefactor for pop incorrect");
}

void movsum_uw_t::resum() {
    // accumulator must be set to zero
    sum = lv_32fc_t(0.0f, 0.0f);

    for (uint32_t i = 0; i < uw.size(); ++i) {
        // how many elements do we need to jump back from ptr?
        const uint32_t n_backward = length - i * n_repelem;

        const uint32_t B = get_front_idx(n_backward);

        lv_32fc_t sum_partial;

        // we don't need to wrap
        if (B + n_repelem <= length) {
            volk_32fc_accumulator_s32fc(&sum_partial, &shiftreg[B], n_repelem);
        } else {
            // first half
            volk_32fc_accumulator_s32fc(&sum_partial, &shiftreg[B], length - B);

            // second half
            lv_32fc_t sec_half(0.0f, 0.0f);
            volk_32fc_accumulator_s32fc(&sec_half, &shiftreg[0], n_repelem - (length - B));
            sum_partial += sec_half;
        }

        sum += uw[i] * sum_partial;
    }
}

void movsum_uw_t::pop_push(const lv_32fc_t val) {
    sum -= prefactor_pop * shiftreg[ptr];
    shiftreg[ptr] = prefactor_push * val;
    sum += shiftreg[ptr];

    // phase changes within accumulator
    for (auto n_backward : uw_pos_neg_idx) {
        sum -= 2.0f * shiftreg[get_front_idx(n_backward)];
    }
    for (auto n_backward : uw_neg_pos_idx) {
        sum += 2.0f * shiftreg[get_front_idx(n_backward)];
    }

    ++ptr;
    ptr %= length;
}

std::vector<uint32_t> movsum_uw_t::get_uw_changes_idx(const std::vector<float>& uw,
                                                      const uint32_t n_repelem,
                                                      const bool pos_neg) {
    std::vector<uint32_t> ret;

    const float second = pos_neg ? 1.0f : -1.0f;
    const float first = -second;

    /*  uw:               1      -1       1       1      -1      -1      -1
     *  uw:                  -1      -1       1      -1       1       1
     *  indices:              0       1       2       3       4       5
     *
     *  uw with 2 reps:      -1  -1  -1  -1   1   1  -1  -1   1   1   1   1
     *  index:                0   1   2   3   4   5   6   7   8   9  10  11
     */

    const uint32_t n_elem = uw.size() * n_repelem;

    // go over entire cover sequence and find sign changes
    for (uint32_t i = 1; i < uw.size(); ++i) {
        if (uw[i - 1] == first && uw[i] == second) {
            // offset relative to last element
            ret.push_back(n_elem - i * n_repelem);
        }
    }

    dectnrp_assert(
        !(std::all_of(uw.begin(), uw.end(), [&](const auto x) { return x == uw.front(); }) &&
          ret.size() > 0),
        "uw values are all the same, yet there are sign changes");

    return ret;
}

}  // namespace dectnrp::phy
