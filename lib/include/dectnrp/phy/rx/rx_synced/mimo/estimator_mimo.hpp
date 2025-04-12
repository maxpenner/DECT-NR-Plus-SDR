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

#include "dectnrp/phy/rx/rx_synced/estimator/estimator.hpp"
#include "dectnrp/phy/rx/rx_synced/mimo/mimo_report.hpp"
#include "dectnrp/sections_part3/beamforming_and_antenna_port_mapping.hpp"

namespace dectnrp::phy {

class estimator_mimo_t final : public estimator_t {
    public:
        estimator_mimo_t(const uint32_t N_RX_, const uint32_t N_TS_max_);
        ~estimator_mimo_t();

        estimator_mimo_t() = delete;
        estimator_mimo_t(const estimator_mimo_t&) = delete;
        estimator_mimo_t& operator=(const estimator_mimo_t&) = delete;
        estimator_mimo_t(estimator_mimo_t&&) = delete;
        estimator_mimo_t& operator=(estimator_mimo_t&&) = delete;

        /**
         * \brief Actually not used, as the STF does not allow a complete channel measurement for
         * packets with more than one transmit stream.
         *
         * \param channel_antennas
         */
        virtual void process_stf(const channel_antennas_t& channel_antennas,
                                 const process_stf_meta_t& process_stf_meta) override final;

        /**
         * \brief Called once at the end of a packet.
         *
         * \param channel_antennas
         * \param process_drs_meta
         */
        virtual void process_drs(const channel_antennas_t& channel_antennas,
                                 const process_drs_meta_t& process_drs_meta) override final;

        [[nodiscard]] const mimo_report_t& get_mimo_report() const;

    private:
        virtual void reset_internal() override final;

        const uint32_t N_RX;
        const uint32_t N_TS_max;

        // beamforming matrix LUT
        section3::W_t W;

        // working copy
        mimo_report_t mimo_report{};

        // wideband spectrum subcarrier spacing and offset
        uint32_t step_width;
        uint32_t step_offset;

        // for SIMD
        std::vector<cf_t*> stage_rx_ts_vec;
        std::vector<cf_t*> stage_rx_ts_transpose_vec;
        cf_t* stage_multiplication;

        void set_stages(const channel_antennas_t& channel_antennas,
                        const process_drs_meta_t& process_drs_meta);

        [[nodiscard]] static uint32_t mode_single_spatial_stream_3_7(
            const section3::W_t& W,
            const uint32_t N_TX_virt,
            const uint32_t N_RX_virt,
            const std::vector<cf_t*>& stage,
            cf_t* stage_multiplication);
};

}  // namespace dectnrp::phy
