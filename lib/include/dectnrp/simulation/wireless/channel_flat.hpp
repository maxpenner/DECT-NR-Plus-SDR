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

#include <string>
#include <vector>

#include "dectnrp/simulation/wireless/channel.hpp"

namespace dectnrp::simulation {

class channel_flat_t final : public channel_t {
    public:
        explicit channel_flat_t(const uint32_t id_0_,
                                const uint32_t id_1_,
                                const uint32_t samp_rate_,
                                const uint32_t spp_size_,
                                const uint32_t nof_antennas_0_,
                                const uint32_t nof_antennas_1_);
        ~channel_flat_t() = default;

        channel_flat_t() = delete;
        channel_flat_t(const channel_flat_t&) = delete;
        channel_flat_t& operator=(const channel_flat_t&) = delete;
        channel_flat_t(channel_flat_t&&) = delete;
        channel_flat_t& operator=(channel_flat_t&&) = delete;

        static const std::string name;

        /**
         * \brief superimpose signal from virtual space
         *
         * \param vspptx own transmission
         * \param vspprx own reception
         * \param vspptx_other other simulator's transmission to be superimposed onto vspprx
         */
        void superimpose(const vspptx_t& vspptx,
                         vspprx_t& vspprx,
                         const vspptx_t& vspptx_other) const override final;

        void randomize_small_scale() override final;

        const uint32_t nof_antennas_0;
        const uint32_t nof_antennas_1;

    private:
        std::vector<cf_t> coeffs;
};

}  // namespace dectnrp::simulation
