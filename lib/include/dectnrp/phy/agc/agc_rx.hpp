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

#include "dectnrp/common/ant.hpp"
#include "dectnrp/phy/agc/agc.hpp"

namespace dectnrp::phy::agc {

class agc_rx_t final : public agc_t {
    public:
        agc_rx_t() = default;

        /**
         * \brief Software AGC
         *
         * \param agc_config_ basic AGC settings
         * \param rms_target_ RMS target the AGC is trying to hold (typically 0.1 to 0.3)
         * \param sensitivity_offset_max_dB_ maximum additional sensitivity for any antenna relative
         * to least sensitive antenna (positive number)
         */
        explicit agc_rx_t(const agc_config_t agc_config_,
                          const float rms_target_,
                          const float sensitivity_offset_max_dB_);

        /**
         * \brief With an SDR, gain settings can't be applied quasi-instantaneously with
         * deterministic latencies. Instead, UHD allows gain changes to be made at a specific point
         * in time in the future. Until that time has been reached, the old gain value is still in
         * effect. With this function we can save a pending gain change. Only one change can be
         * pending.
         *
         * \param power_ant_0dBFS_pending_ gain value that will be effective shortly
         * \param power_ant_0dBFS_pending_time_64_ time when above value will become effective
         */
        void set_power_ant_0dBFS_pending(
            const common::ant_t& power_ant_0dBFS_pending_,
            const int64_t power_ant_0dBFS_pending_time_64_ = common::adt::UNDEFINED_EARLY_64);

        /**
         * \brief Get current gain values. Internally also checks if a possibly pending value has
         * taken effect in the meantime.
         *
         * \param now_64 time of request
         * \return
         */
        const common::ant_t& get_power_ant_0dBFS(const int64_t now_64) const;

        /**
         * \brief Takes the measured RMS of the input signal, and calculates the required gain
         * change to achieve the target RMS. The change is limited is size. This is a software AGC,
         * and as such inherently slow.
         *
         * \param t_64
         * \param rms_measured_
         * \return Returns a positive step if rms_measured_ > rms_target, which means radio hardware
         * has to INCREASE the rx power at 0dBFS, and by that become less sensitive. Returns a
         * negative step if rms_measured_ < rms_target, which means radio hardware has to DECREASE
         * the rx power at 0dBFS, and by that become more sensitive. Returns 0.0f if protection
         * duration has not passed yet or no change is required.
         */
        const common::ant_t get_gain_step_dB(const common::ant_t& rx_power_ant_0dBFS,
                                             const common::ant_t& rms_measured_);

        float get_rms_target() const { return rms_target; };

        const common::ant_t& get_rms_measured_last_known() const {
            return rms_measured_last_known;
        };

    private:
        mutable common::ant_t power_ant_0dBFS;
        common::ant_t power_ant_0dBFS_pending;

        float rms_target;
        common::ant_t rms_measured_last_known;

        float sensitivity_offset_max_dB;
};

}  // namespace dectnrp::phy::agc
