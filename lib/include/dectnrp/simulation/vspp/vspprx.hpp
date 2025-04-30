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

#include <cstdint>
#include <vector>

#include "dectnrp/common/ant.hpp"
#include "dectnrp/common/complex.hpp"
#include "dectnrp/simulation/topology/position.hpp"
#include "dectnrp/simulation/vspp/vspp.hpp"

namespace dectnrp::simulation {

class vspprx_t final : public vspp_t {
    public:
        explicit vspprx_t(const uint32_t id_,
                          const uint32_t nof_antennas_,
                          const uint32_t samp_rate_,
                          const uint32_t spp_size_);
        ~vspprx_t() = default;

        vspprx_t() = delete;
        vspprx_t(const vspprx_t&) = delete;
        vspprx_t& operator=(const vspprx_t&) = delete;
        vspprx_t(vspprx_t&&) = delete;
        vspprx_t& operator=(vspprx_t&&) = delete;

        void spp_read(std::vector<cf_t*>& out, uint32_t nof_samples) const;

        /// same as above, but for void pointer (legacy code from USRP UHD)
        void spp_read_v(std::vector<void*>& out, uint32_t nof_samples) const;

        /// additional meta data to be set by hw_simulator RX threads before entering vspace
        struct meta_t {
                int64_t now_64;

                common::ant_t rx_power_ant_0dBFS{};
                float rx_noise_figure_dB;
                float rx_snr_in_net_bandwidth_norm_dB;

                void set_reasonable_default_values(const uint32_t nof_antennas);
        } meta;
};

}  // namespace dectnrp::simulation
