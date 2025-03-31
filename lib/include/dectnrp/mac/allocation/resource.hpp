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

#include "dectnrp/sections_part3/derivative/duration.hpp"

namespace dectnrp::mac::allocation {

class resource_t {
    public:
        /**
         * \brief An allocation must have an offset relative to the beginning of a frame, and a
         * length starting at that offset. For both, we use the class duration_t to have lengths in
         * all relevant units.
         *
         * \param offset_
         * \param length_
         */
        explicit resource_t(const section3::duration_t offset_, const section3::duration_t length_);

        section3::duration_t offset;
        section3::duration_t length;

        uint32_t get_first_sample_index() const;
        uint32_t get_last_sample_index() const;

        bool is_positive_length(const uint32_t length_ = 1) const;
        bool is_causal() const;
        bool is_well_defined() const;

        bool is_touching_earlier(const resource_t& other) const;
        bool is_touching_later(const resource_t& other) const;
        bool is_orthogonal_earlier(const resource_t& other) const;
        bool is_orthogonal_later(const resource_t& other) const;
        bool is_orthogonal(const resource_t& other) const;
        bool is_leq(const resource_t& other) const;
        bool is_seq(const resource_t& other) const;
};

}  // namespace dectnrp::mac::allocation
