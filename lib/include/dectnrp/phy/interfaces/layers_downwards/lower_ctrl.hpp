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
#include <memory>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/phy/agc/agc_rx.hpp"
#include "dectnrp/phy/agc/agc_tx.hpp"
#include "dectnrp/phy/harq/process_pool.hpp"
#include "dectnrp/phy/pool/job_queue.hpp"
#include "dectnrp/phy/worker_pool_config.hpp"
#include "dectnrp/radio/hw.hpp"
#include "dectnrp/sections_part3/derivative/duration_lut.hpp"

namespace dectnrp::phy {

class lower_ctrl_t {
    public:
        /**
         * \brief Interface for tpoint (MAC layer and above) to control one pair of hw on
         * the radio layer and worker_pool on PHY (both part of lower).
         *
         * \param hw_ change frequency, gain etc.
         * \param worker_pool_config_ contains radio device class etc.
         * \param job_queue_ directly write jobs
         */
        lower_ctrl_t(radio::hw_t& hw_,
                     const phy::worker_pool_config_t& worker_pool_config_,
                     phy::job_queue_t& job_queue_)
            : hw(hw_),
              buffer_rx(*hw.buffer_rx.get()),
              worker_pool_config(worker_pool_config_),
              job_queue(job_queue_),
              hpp(std::make_shared<phy::harq::process_pool_t>(
                  worker_pool_config.maximum_packet_sizes, 8, 8)),
              duration_lut(sp3::duration_lut_t(hw.get_samp_rate())),
              // generic values for a software TX AGC
              agc_tx(phy::agc::agc_tx_t(
                  phy::agc::agc_config_t{buffer_rx.nof_antennas, 1.0f, 5.0f, 2.0f},
                  phy::agc::agc_t::OFDM_AMPLITUDE_FACTOR_MINUS_15dB,
                  -55.0f)),
              // generic values for a software RX AGC
              agc_rx(phy::agc::agc_rx_t(
                  phy::agc::agc_config_t{buffer_rx.nof_antennas, 1.0f, 8.0f, 2.0f},
                  phy::agc::agc_rx_mode_t::tune_individually,
                  phy::agc::agc_t::OFDM_AMPLITUDE_FACTOR_MINUS_20dB,
                  15.0f)) {}

        radio::hw_t& hw;
        const radio::buffer_rx_t& buffer_rx;
        const phy::worker_pool_config_t& worker_pool_config;
        phy::job_queue_t& job_queue;

        std::shared_ptr<phy::harq::process_pool_t> hpp;

        /**
         * \brief Lookup table (lut) to convert from generic duration enums to equivalent number of
         * samples. This conversion requires knowledge of the hardware sample rate, which becomes
         * available at runtime.
         */
        const sp3::duration_lut_t duration_lut;

        /// AGC of transmitter path
        phy::agc::agc_tx_t agc_tx;

        /// AGC of receiver path
        phy::agc::agc_rx_t agc_rx;

        /**
         * \brief Every transmission/packet on the PHY + radio layer has a unique 64-bit ID.
         * It starts at 0 and from transmission to transmission, the ID must be strictly
         * increasing for packets that are transmitted later.
         */
        int64_t tx_order_id{0};

        /**
         * \brief A strictly increasing time before which we are no longer allowed to
         * transmit. The initial value lies very far in the past, so the first transmission
         * is guaranteed to start after it.
         */
        int64_t tx_earliest_64{common::adt::UNDEFINED_EARLY_64};
};

}  // namespace dectnrp::phy
