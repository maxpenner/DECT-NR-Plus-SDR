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

#include "dectnrp/common/ant.hpp"

#include <algorithm>
#include <numeric>
#include <span>

#include "dectnrp/common/prog/assert.hpp"

#define ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE \
    dectnrp_assert(0 < nof_antennas, "number of antennas is zero")

#define ASSERT_INDEX_WITHIN_NUMBER_OF_ANTENNAS \
    dectnrp_assert(idx < nof_antennas, "idx out-of-bound")

namespace dectnrp::common {

ant_t::ant_t(const std::size_t nof_antennas_)
    : nof_antennas(nof_antennas_) {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    dectnrp_assert(nof_antennas <= limits::dectnrp_max_nof_antennas, "too large");
};

std::size_t ant_t::get_nof_antennas() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return nof_antennas;
}

float& ant_t::at(const std::size_t idx) {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;
    ASSERT_INDEX_WITHIN_NUMBER_OF_ANTENNAS;

    return ary.at(idx);
};

float ant_t::at(const std::size_t idx) const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;
    ASSERT_INDEX_WITHIN_NUMBER_OF_ANTENNAS;

    return ary.at(idx);
};

void ant_t::fill(const float val) {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    std::fill(ary.begin(), ary.begin() + nof_antennas, val);
}

const ant_t::ary_t& ant_t::get_ary() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return ary;
};

bool ant_t::has_any_larger(const float threshold) const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return std::any_of(ary.begin(), ary.begin() + nof_antennas, [threshold](const auto& val) {
        return val > threshold;
    });
}

std::size_t ant_t::get_nof_larger(const float threshold) const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return std::count_if(ary.begin(), ary.begin() + nof_antennas, [threshold](const auto& val) {
        return val > threshold;
    });
}

float ant_t::get_sum() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return std::accumulate(ary.begin(), ary.begin() + nof_antennas, 0.0f);
}

float ant_t::get_max() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return *std::max_element(ary.begin(), ary.begin() + nof_antennas);
}

std::size_t ant_t::get_index_of_max() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return std::max_element(ary.begin(), ary.begin() + nof_antennas) - ary.begin();
}

float ant_t::get_min() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return *std::min_element(ary.begin(), ary.begin() + nof_antennas);
}

std::size_t ant_t::get_index_of_min() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    return std::min_element(ary.begin(), ary.begin() + nof_antennas) - ary.begin();
}

std::string ant_t::get_readable_list() const {
    ASSERT_NUMBER_OF_ANTENNAS_IS_POSITIVE;

    std::string ret = "[";

    for (auto elem : std::span(ary).first(nof_antennas)) {
        ret += std::to_string(elem) + ",";
    }

    ret.pop_back();

    ret += "]";

    return ret;
}

}  // namespace dectnrp::common
