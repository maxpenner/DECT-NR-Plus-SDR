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

#include "dectnrp/phy/agc/agc.hpp"

namespace dectnrp::phy::agc {

class agc_tx_t final : public agc_t {
    public:
        agc_tx_t() = default;

        /**
         * \brief Software AGC
         *
         * \param agc_config_ basic AGC settings
         * \param ofdm_amplitude_factor_ OFDM amplitude reduction (typically -10 to -20dB)
         * \param rx_dBm_target_ target receive power (typically -40 to -80dBm)
         */
        explicit agc_tx_t(const agc_config_t agc_config_,
                          const float ofdm_amplitude_factor_,
                          const float rx_dBm_target_);

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
            const float power_ant_0dBFS_pending_,
            const int64_t power_ant_0dBFS_pending_time_64_ = common::adt::UNDEFINED_EARLY_64);

        /**
         * \brief Get current gain values. Internally also checks if a possibly pending value has
         * taken effect in the meantime.
         *
         * \param now_64
         * \return
         */
        const float get_power_ant_0dBFS(const int64_t now_64) const;

        /**
         * \brief TX gain change required to achieve rx_dBm_target at the opposite site.
         *
         * \param t_64
         * \param tx_dBm_opposite
         * \param tx_power_ant_0dBFS
         * \param rx_power_ant_0dBFS
         * \param rms_measured_
         * \return Returns a positive step if radio hardware has to INCREASE the tx power at 0dBFS.
         * Returns a negative step if radio hardware has to DECREASE the rx power at 0dBFS. Returns
         * 0.0f if protection duration has not passed yet or no change is required.
         */
        float get_gain_step_dB(const int64_t t_64,
                               const float tx_dBm_opposite,
                               const float tx_power_ant_0dBFS,
                               const common::ant_t& rx_power_ant_0dBFS,
                               const common::ant_t& rms_measured_);

        float get_ofdm_amplitude_factor() const { return ofdm_amplitude_factor; };

    private:
        /// gain settings are applied in the future, keeps track of what value is effective
        mutable float power_ant_0dBFS;
        float power_ant_0dBFS_pending;

        float ofdm_amplitude_factor;
        float rx_dBm_target;
};

}  // namespace dectnrp::phy::agc
