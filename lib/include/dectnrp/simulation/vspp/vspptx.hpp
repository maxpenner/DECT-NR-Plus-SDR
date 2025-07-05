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
#include "dectnrp/simulation/topology/trajectory.hpp"
#include "dectnrp/simulation/vspp/vspp.hpp"

namespace dectnrp::simulation {

class vspptx_t final : public vspp_t {
    public:
        explicit vspptx_t(const uint32_t id_,
                          const uint32_t nof_antennas_,
                          const uint32_t samp_rate_,
                          const uint32_t spp_size_);
        ~vspptx_t() = default;

        vspptx_t() = delete;
        vspptx_t(const vspptx_t&) = delete;
        vspptx_t& operator=(const vspptx_t&) = delete;
        vspptx_t(vspptx_t&&) = delete;
        vspptx_t& operator=(vspptx_t&&) = delete;

        void spp_write(const std::vector<cf_t*>& inp, uint32_t offset, uint32_t nof_samples);

        /// same as above, but for void pointer (legacy code from USRP UHD)
        void spp_write_v(const std::vector<void*>& inp, uint32_t offset, uint32_t nof_samples);

        /// hw_simulator TX threads use deep copy when accessing vspace
        void deepcopy(vspptx_t& dst) const;

        /// mark parts with non-zero TX samples
        int32_t tx_idx;                     // where does TX signal start within spp?
        int32_t tx_length;                  // how many TX samples follow after the start?
        void tx_set_no_non_zero_samples();  // mark spp as not containing non-zero TX samples

        /// additional meta data to be set by hw_simulator TX threads before entering vspace
        struct meta_t {
                bool vspprx_counterpart_registered;

                int64_t now_64;

                /// common for TX and RX
                topology::position_t position;
                topology::trajectory_t trajectory;
                float freq_Hz;
                float net_bandwidth_norm;

                /// TX specific
                common::ant_t tx_power_ant_0dBFS{};
                /// positive values mean that the leaked power into TX is smaller than the
                /// TX power, internally it is treated like large scale fading
                float tx_into_rx_leakage_dB;

                void set_reasonable_default_values();
        } meta;
};

}  // namespace dectnrp::simulation
