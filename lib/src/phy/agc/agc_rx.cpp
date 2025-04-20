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
#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/agc/agc_rx_param.hpp"

namespace dectnrp::phy::agc {

agc_rx_t::agc_rx_t(const agc_config_t agc_config_,
                   const float rms_target_,
                   const float sensitivity_offset_max_dB_)
    : agc_t(agc_config_),
      rms_target(rms_target_),
      sensitivity_offset_max_dB(sensitivity_offset_max_dB_) {
    // define number of antennas
    rms_measured_last_known = common::ant_t(agc_config.nof_antennas);

    // fill with target value as if AGC is perfectly tuned
    rms_measured_last_known.fill(rms_target);

    dectnrp_assert(OFDM_AMPLITUDE_FACTOR_MINUS_20dB <= rms_target, "too small");
    dectnrp_assert(rms_target <= OFDM_AMPLITUDE_FACTOR_MINUS_00dB, "too large");
    dectnrp_assert(0.0f <= sensitivity_offset_max_dB, "too small");
    dectnrp_assert(sensitivity_offset_max_dB <= 20.0f, "too large");
}

void agc_rx_t::set_power_ant_0dBFS_pending(const common::ant_t& power_ant_0dBFS_pending_,
                                           const int64_t power_ant_0dBFS_pending_time_64_) {
    // is this an immediate gain change?
    if (power_ant_0dBFS_pending_time_64_ < 0) {
        // change is effective now
        power_ant_0dBFS = power_ant_0dBFS_pending_;

        // indicate that we don't have a pending value
        power_ant_0dBFS_pending_time_64 = common::adt::UNDEFINED_EARLY_64;
    } else {
        // save pending value
        power_ant_0dBFS_pending = power_ant_0dBFS_pending_;
        power_ant_0dBFS_pending_time_64 = power_ant_0dBFS_pending_time_64_;
    }
}

const common::ant_t& agc_rx_t::get_power_ant_0dBFS(const int64_t now_64) const {
    // do we have a pending value?
    if (0 <= power_ant_0dBFS_pending_time_64) {
        // have we reached the time of the pending gain change?
        if (power_ant_0dBFS_pending_time_64 <= now_64) {
            // change is effective now
            power_ant_0dBFS = power_ant_0dBFS_pending;

            // indicate that we no longer have a pending value
            power_ant_0dBFS_pending_time_64 = common::adt::UNDEFINED_EARLY_64;
        }
    }

    return power_ant_0dBFS;
}

const common::ant_t agc_rx_t::get_gain_step_dB(const int64_t t_64,
                                               const common::ant_t& rx_power_ant_0dBFS,
                                               const common::ant_t& rms_measured_) {
    dectnrp_assert(rx_power_ant_0dBFS.get_nof_antennas() == agc_config.nof_antennas,
                   "incorrect number of antennas");
    dectnrp_assert(rms_measured_.get_nof_antennas() == agc_config.nof_antennas,
                   "incorrect number of antennas");

    // save value, even if protection duration is still active
    rms_measured_last_known = rms_measured_;

    // return zero gain change if gain is protected
    if (!check_protect_duration_passed(t_64)) {
        return common::ant_t{};
    } else {
        protect_duration_start_64 = t_64;
    }

    // what is the RX power at 0dBFS for the antenna with the lowest sensitivity?
    const float A = rx_power_ant_0dBFS.get_max();

    // how far can we reduce the sensitivity? the goal is to avoid RX sensitivities to differ too
    // much
    const float B = A - sensitivity_offset_max_dB;

#ifdef PHY_AGC_RX_ANTENNA_SENSITIVITY_INDIVIDUALLY_OR_EQUAL

    // calculate arbitrary step for every antenna
    common::ant_t arbitrary_gain_step_dB(agc_config.nof_antennas);
    for (size_t i = 0; i < agc_config.nof_antennas; ++i) {
        /* Not every antenna necessarily has a valid RMS value. This can happen if synchronization
         * did not detect a valid peak.
         *
         * If there was a peak and thus the RMS value is valid (i.e. > 0), C is the arbitrary gain
         * step required to achieve the target RMS.
         *
         * If there was no peak and thus the RMS value is invalid (i.e. == 0), C is set to the gain
         * step required to achieve the same sensitivity as A. Thus, the antenna will move towards a
         * lower sensitivity since A >= rx_power_ant_0dBFS.at(i) for all i.
         *
         *      Assume A = -30dBm and rx_power_ant_0dBFS.at(i) = -50dBm. Then C = 20dB.
         *      Assume A = -40dBm and rx_power_ant_0dBFS.at(i) = -40dBm. Then C = 0dB.
         */
        const float C = (rms_measured_last_known.at(i) > 0)
                            ? common::adt::mag2db(rms_measured_last_known.at(i) / rms_target)
                            : A - rx_power_ant_0dBFS.at(i);

        /* By how much are we allowed to reduce the sensitivity for this antenna?
         *
         * Assume B = -52dBm and rx_power_ant_0dBFS.at(i) = -30dBm. Then the allowed step is
         * -52-(-30) = -22dB, i.e. the antenna may become more sensitive.
         *
         * Assume B = -44dBm and rx_power_ant_0dBFS.at(i) = -60dBm. Then the allowed step is
         * -44-(-60) = +24dB, i.e. the antenna must become less sensitive.
         */
        const float D = B - rx_power_ant_0dBFS.at(i);

        // take the larger of both values
        arbitrary_gain_step_dB.at(i) = std::max(C, D);
    }

#else

    const auto idx_max = rms_measured_last_known.get_index_of_max();

    const float C = common::adt::mag2db(rms_measured_last_known.at(idx_max) / rms_target);

    const float D = B - rx_power_ant_0dBFS.at(idx_max);

    // take the larger of both values
    const float arbitrary_gain_step_dB_equal = std::max(C, D);

    // calculate arbitrary step for every antenna
    common::ant_t arbitrary_gain_step_dB(agc_config.nof_antennas);
    for (size_t i = 0; i < agc_config.nof_antennas; ++i) {
        arbitrary_gain_step_dB.at(i) = arbitrary_gain_step_dB_equal;
    }

#endif

    return quantize_and_limit_gain_step_dB(arbitrary_gain_step_dB);
}

}  // namespace dectnrp::phy::agc
