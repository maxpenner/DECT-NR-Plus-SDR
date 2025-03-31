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

#include <memory>
#include <string>

#include "dectnrp/simulation/wireless/channel.hpp"
#include "dectnrp/simulation/wireless/link.hpp"

namespace dectnrp::simulation {

class channel_doubly_t final : public channel_t {
    public:
        explicit channel_doubly_t(const uint32_t id_0_,
                                  const uint32_t id_1_,
                                  const uint32_t samp_rate_,
                                  const uint32_t spp_size_,
                                  const uint32_t nof_antennas_0_,
                                  const uint32_t nof_antennas_1_,
                                  const std::string sim_channel_name_intxx_);
        ~channel_doubly_t() = default;

        channel_doubly_t() = delete;
        channel_doubly_t(const channel_doubly_t&) = delete;
        channel_doubly_t& operator=(const channel_doubly_t&) = delete;
        channel_doubly_t(channel_doubly_t&&) = delete;
        channel_doubly_t& operator=(channel_doubly_t&&) = delete;

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
        /**
         * \brief While a channel_doubly_t connects two devices which can have different numbers of
         * antennas, a link_t connects two antennas. If, for instance, nof_antennas_0=2, and
         * nof_antennas_1=4, we need a total of 8 links. Every link is full duplex, i.e. both
         * devices can transmit at the same time.
         */
        std::vector<std::unique_ptr<link_t>> link_vec;

        /**
         * \brief To configure the doubly selective channel, we have to extract the channel
         * properties from the string sim_channel_name_intxx_. An example of this string is
         * "doubly_2_10_100".
         *
         * It consists of four elements delimited by an underscore:
         *
         * 1. doubly
         * 2. pdp_idx = 2UL
         * 3. tau_rms_ns = 10.0f
         * 4. fD_Hz = 100.0f
         */
        static const std::string delimiter;
        void parse_and_init_links(std::string arg);
};

}  // namespace dectnrp::simulation
