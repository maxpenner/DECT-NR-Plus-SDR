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

#include <cstdint>

#include "dectnrp/common/complex.hpp"
#include "dectnrp/simulation/vspp/vspprx.hpp"
#include "dectnrp/simulation/vspp/vspptx.hpp"

namespace dectnrp::simulation {

class channel_t {
    public:
        explicit channel_t(const uint32_t id_0_,
                           const uint32_t id_1_,
                           const uint32_t samp_rate_,
                           const uint32_t spp_size_);
        virtual ~channel_t();

        channel_t() = delete;
        channel_t(const channel_t&) = delete;
        channel_t& operator=(const channel_t&) = delete;
        channel_t(channel_t&&) = delete;
        channel_t& operator=(channel_t&&) = delete;

        /**
         * \brief superimpose signal from virtual space
         *
         * \param vspptx own transmission
         * \param vspprx own reception
         * \param vspptx_other other simulator's transmission to be superimposed onto vspprx
         */
        virtual void superimpose(const vspptx_t& vspptx,
                                 vspprx_t& vspprx,
                                 const vspptx_t& vspptx_other) const = 0;

        /// create new and independent channel impulse response
        virtual void randomize_small_scale() = 0;

        /// large scale fading based on position and 0dBFS settings
        static float get_large_scale_via_pathloss(const vspptx_t& vspptx,
                                                  const vspprx_t& vspprx,
                                                  const vspptx_t& vspptx_other,
                                                  const size_t tx_idx,
                                                  const size_t rx_idx);

        const uint32_t id_0;
        const uint32_t id_1;
        const uint32_t samp_rate;
        const uint32_t spp_size;

    protected:
        cf_t* large_scale_stage;
        cf_t* small_scale_stage;

        bool are_args_valid(const vspptx_t& vspptx, const vspprx_t& vspprx) const;
};

}  // namespace dectnrp::simulation
