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

#include <concepts>
#include <cstdint>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/sections_part3/derivative/duration_ec.hpp"

namespace dectnrp::section3 {

class duration_t {
    public:
        duration_t() = default;

        /**
         * \brief The length represented by duration_ec must divide one second without remainder.
         *
         * \param samp_rate_ hardware sample rate
         * \param duration_ec_
         * \param mult_ multiplier
         */
        explicit duration_t(const uint32_t samp_rate_,
                            const duration_ec_t duration_ec_,
                            const uint32_t mult_ = 1);

        friend bool operator>(duration_t& lhs, duration_t& rhs) {
            return lhs.N_samples > rhs.N_samples ? true : false;
        }

        friend bool operator<(duration_t& lhs, duration_t& rhs) {
            return lhs.N_samples < rhs.N_samples ? true : false;
        }

        template <std::integral T = uint32_t>
        T get_samp_rate() const {
            return static_cast<T>(samp_rate);
        }

        template <std::integral T = uint32_t>
        T get_N_samples() const {
            return static_cast<T>(N_samples);
        }

    private:
        uint32_t samp_rate{};

        [[maybe_unused]] duration_ec_t duration_ec{};
        [[maybe_unused]] uint32_t mult{};

        uint32_t N_samples{};
};

}  // namespace dectnrp::section3
