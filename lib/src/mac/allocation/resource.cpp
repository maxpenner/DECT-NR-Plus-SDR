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

#include "dectnrp/mac/allocation/resource.hpp"

#include <algorithm>
#include <limits>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"

namespace dectnrp::mac::allocation {

resource_t::resource_t(const section3::duration_t offset_, const section3::duration_t length_)
    : offset(offset_),
      length(length_) {
    dectnrp_assert(is_well_defined(), "not well defined");
}

uint32_t resource_t::get_first_sample_index() const { return offset.get_N_samples(); }

uint32_t resource_t::get_last_sample_index() const {
    return offset.get_N_samples() + length.get_N_samples() - 1;
}

bool resource_t::is_positive_length(const uint32_t length_) const {
    return length_ <= length.get_N_samples();
}

bool resource_t::is_causal() const { return get_first_sample_index() <= get_last_sample_index(); }

bool resource_t::is_well_defined() const { return is_positive_length() && is_causal(); }

bool resource_t::is_touching_earlier(const resource_t& other) const {
    dectnrp_assert(other.is_well_defined(), "not well defined");
    return other.get_last_sample_index() + 1 == get_first_sample_index();
}

bool resource_t::is_touching_later(const resource_t& other) const {
    dectnrp_assert(other.is_well_defined(), "not well defined");
    return get_last_sample_index() + 1 == other.get_first_sample_index();
}

bool resource_t::is_orthogonal_earlier(const resource_t& other) const {
    dectnrp_assert(other.is_well_defined(), "not well defined");
    return other.get_last_sample_index() < get_first_sample_index();
}

bool resource_t::is_orthogonal_later(const resource_t& other) const {
    dectnrp_assert(other.is_well_defined(), "not well defined");
    return get_last_sample_index() < other.get_first_sample_index();
}

bool resource_t::is_orthogonal(const resource_t& other) const {
    dectnrp_assert(other.is_well_defined(), "not well defined");
    return is_orthogonal_earlier(other) || is_orthogonal_later(other);
}

bool resource_t::is_leq(const resource_t& other) const {
    dectnrp_assert(other.is_well_defined(), "not well defined");
    return other.length.get_N_samples() <= length.get_N_samples();
}

bool resource_t::is_seq(const resource_t& other) const {
    dectnrp_assert(other.is_well_defined(), "not well defined");
    return length.get_N_samples() <= other.length.get_N_samples();
}

}  // namespace dectnrp::mac::allocation
