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

#include <array>
#include <string>

#include "dectnrp/limits.hpp"

namespace dectnrp::common {

/**
 * \brief Wrapper to contain floating point values representing some antenna property, for instance
 * RMS or a gain settings. Quite useful in many places of the SDR.
 */
class ant_t {
    public:
        ant_t() = default;
        explicit ant_t(const std::size_t nof_antennas_);

        typedef std::array<float, limits::dectnrp_max_nof_antennas> ary_t;

        [[nodiscard]] std::size_t get_nof_antennas() const;

        /// convenience wrapper to same-name functions of ary
        [[nodiscard]] float& at(const std::size_t idx);
        [[nodiscard]] float at(const std::size_t idx) const;
        void fill(const float val);

        /// getter for read-only version of array
        [[nodiscard]] const ary_t& get_ary() const;

        /// convenience functions to extract specific values
        [[nodiscard]] bool has_any_larger(const float threshold) const;
        [[nodiscard]] std::size_t get_nof_larger(const float threshold) const;
        [[nodiscard]] float get_sum() const;
        [[nodiscard]] float get_max() const;
        [[nodiscard]] std::size_t get_index_of_max() const;
        [[nodiscard]] float get_min() const;
        [[nodiscard]] std::size_t get_index_of_min() const;

        [[nodiscard]] std::string get_readable_list() const;

    private:
        /// used number of antennas
        std::size_t nof_antennas{};

        /// array with float property of each antenna
        ary_t ary{};
};

}  // namespace dectnrp::common
