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

#include "dectnrp/phy/agc/agc_rx.hpp"

#include <cmath>

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::agc {

agc_rx_t::agc_rx_t(const agc_config_t agc_config_,
                   const agc_rx_mode_t agc_rx_mode_,
                   const float rms_target_,
                   const float sensitivity_offset_max_dB_)
    : agc_t(agc_config_),
      agc_rx_mode(agc_rx_mode_),
      rms_target(rms_target_),
      sensitivity_offset_max_dB(sensitivity_offset_max_dB_) {
    dectnrp_assert(OFDM_AMPLITUDE_FACTOR_MINUS_20dB <= rms_target, "too small");
    dectnrp_assert(rms_target <= OFDM_AMPLITUDE_FACTOR_MINUS_00dB, "too large");
    dectnrp_assert(0.0f <= sensitivity_offset_max_dB, "too small");
    dectnrp_assert(sensitivity_offset_max_dB <= 20.0f, "too large");
}

const common::ant_t agc_rx_t::get_gain_step_dB(const common::ant_t& rx_power_ant_0dBFS,
                                               const common::ant_t& rms_measured) {
    dectnrp_assert(rx_power_ant_0dBFS.get_nof_antennas() == agc_config.nof_antennas,
                   "incorrect number of antennas");
    dectnrp_assert(rms_measured.get_nof_antennas() == agc_config.nof_antennas,
                   "incorrect number of antennas");

    // what is the RX power at 0dBFS for the antenna with the lowest sensitivity?
    const float A = rx_power_ant_0dBFS.get_max();

    // how far can we reduce the sensitivity? the goal is to avoid RX sensitivities to differ too
    // much
    const float B = A - sensitivity_offset_max_dB;

    switch (agc_rx_mode) {
        using enum agc_rx_mode_t;
        case tune_individually:
            {
                // calculate arbitrary step for every antenna
                common::ant_t arbitrary_gain_step_dB(agc_config.nof_antennas);
                for (size_t i = 0; i < agc_config.nof_antennas; ++i) {
                    /* Not every antenna necessarily has a valid RMS value. This can happen if
                     * synchronization did not detect a valid peak.
                     *
                     * If there was a peak and thus the RMS value is valid (i.e. > 0), C is the
                     * arbitrary gain step required to achieve the target RMS.
                     *
                     * If there was no peak and thus the RMS value is invalid (i.e. == 0), C is set
                     * to the gain step required to achieve the same sensitivity as A. Thus, the
                     * antenna will move towards a lower sensitivity since A >=
                     * rx_power_ant_0dBFS.at(i) for all i.
                     *
                     *      Assume A = -30dBm and rx_power_ant_0dBFS.at(i) = -50dBm. Then C = 20dB.
                     *      Assume A = -40dBm and rx_power_ant_0dBFS.at(i) = -40dBm. Then C = 0dB.
                     */
                    const float C = (rms_measured.at(i) > 0)
                                        ? common::adt::mag2db(rms_measured.at(i) / rms_target)
                                        : A - rx_power_ant_0dBFS.at(i);

                    /* By how much are we allowed to reduce the sensitivity for this antenna?
                     *
                     * Assume B = -52dBm and rx_power_ant_0dBFS.at(i) = -30dBm. Then the allowed
                     * step is -52-(-30) = -22dB, i.e. the antenna may become more sensitive.
                     *
                     * Assume B = -44dBm and rx_power_ant_0dBFS.at(i) = -60dBm. Then the allowed
                     * step is -44-(-60) = +24dB, i.e. the antenna must become less sensitive.
                     */
                    const float D = B - rx_power_ant_0dBFS.at(i);

                    // take the larger of both values
                    arbitrary_gain_step_dB.at(i) = std::max(C, D);
                }

                return quantize_and_limit_gain_step_dB(arbitrary_gain_step_dB);
            }

        case tune_collectively:
            {
                const auto idx_max = rms_measured.get_index_of_max();

                const float C = common::adt::mag2db(rms_measured.at(idx_max) / rms_target);

                const float D = B - rx_power_ant_0dBFS.at(idx_max);

                // take the larger of both values
                const float arbitrary_gain_step_dB_equal = std::max(C, D);

                // calculate arbitrary step for every antenna
                common::ant_t arbitrary_gain_step_dB(agc_config.nof_antennas);
                for (size_t i = 0; i < agc_config.nof_antennas; ++i) {
                    arbitrary_gain_step_dB.at(i) = arbitrary_gain_step_dB_equal;
                }

                return quantize_and_limit_gain_step_dB(arbitrary_gain_step_dB);
            }
    }

    dectnrp_assert_failure("unreachable");

    return common::ant_t(agc_config.nof_antennas);
}

}  // namespace dectnrp::phy::agc
