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

#include "dectnrp/phy/agc/agc_tx.hpp"

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::agc {

agc_tx_t::agc_tx_t(const agc_config_t agc_config_,
                   const float ofdm_amplitude_factor_,
                   const float rx_dBm_target_)
    : agc_t(agc_config_),
      ofdm_amplitude_factor(ofdm_amplitude_factor_),
      rx_dBm_target(rx_dBm_target_) {
    dectnrp_assert(OFDM_AMPLITUDE_FACTOR_MINUS_20dB <= ofdm_amplitude_factor, "too small");
    dectnrp_assert(ofdm_amplitude_factor <= OFDM_AMPLITUDE_FACTOR_MINUS_00dB, "too large");
    dectnrp_assert(-80.0f <= rx_dBm_target, "too small");
    dectnrp_assert(rx_dBm_target <= -40.0f, "too large");
}

const common::ant_t agc_tx_t::get_gain_step_dB(const float tx_dBm_opposite,
                                               const common::ant_t& tx_power_ant_0dBFS,
                                               const common::ant_t& rx_power_ant_0dBFS,
                                               const common::ant_t& rms_measured_) {
    // we have to find the antenna with the highest RX power
    float rx_power_dBm_measured_max = -1e6;

    // go over all RX antennas
    for (size_t i = 0; i < agc_config.nof_antennas; ++i) {
        /* Not every antenna necessarily has a valid RMS value. This can happen if synchronization
         * did not detect a valid peak.
         */
        if (rms_measured_.at(i) > 0.0f) {
            // determine RX power
            const float A = rx_power_ant_0dBFS.at(i) + common::adt::mag2db(rms_measured_.at(i));

            // is the RX power of this antenna higher?
            if (rx_power_dBm_measured_max < A) {
                rx_power_dBm_measured_max = A;
            }
        }
    }

    // what would the ideal transmit power at the opposite site have been to reach
    // rx_dBm_target?
    const float tx_dBm_opposite_ideal =
        tx_dBm_opposite + (rx_dBm_target - rx_power_dBm_measured_max);

    // what is our current radiated transmit power?
    const float tx_dBm = tx_power_ant_0dBFS.get_max() + common::adt::mag2db(ofdm_amplitude_factor);

    common::ant_t arbitrary_gain_step_dB(agc_config.nof_antennas);
    for (size_t i = 0; i < agc_config.nof_antennas; ++i) {
        arbitrary_gain_step_dB.at(i) = tx_dBm_opposite_ideal - tx_dBm;
    }

    return quantize_and_limit_gain_step_dB(arbitrary_gain_step_dB);
}

}  // namespace dectnrp::phy::agc
