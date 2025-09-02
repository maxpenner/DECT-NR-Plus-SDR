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

#include "dectnrp/phy/rx/rx_synced/mimo/mimo_report.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_pdu_decoder.hpp"

namespace dectnrp::phy {

class pdc_report_t {
    public:
        explicit pdc_report_t(const sp4::mac_pdu_decoder_t& mac_pdu_decoder_)
            : crc_status(false),
              mac_pdu_decoder(mac_pdu_decoder_),
              snr_dB(0.0f),
              mimo_report(mimo_report_t()) {};

        explicit pdc_report_t(const sp4::mac_pdu_decoder_t& mac_pdu_decoder_,
                              const float snr_dB_,
                              const mimo_report_t mimo_report_)
            : crc_status(true),
              mac_pdu_decoder(mac_pdu_decoder_),
              snr_dB(snr_dB_),
              mimo_report(mimo_report_) {};

        /// 24 bit CRC attached to transport block
        const bool crc_status;

        /// contains all information about the PCC (or PLCF) available
        const sp4::mac_pdu_decoder_t& mac_pdu_decoder;

        /**
         * \brief SNR estimation is based on STF and DRS across all RX antennas a transmit streams.
         * It is the average value across all subcarriers, even when the channel is time-/frequency
         * selective.
         */
        const float snr_dB;

        /**
         * \brief Based on the latest channel estimate, the optimal codebook index is calculated
         * which the transmitter should have used for optimal receive conditions.
         */
        const mimo_report_t mimo_report;
};

}  // namespace dectnrp::phy
