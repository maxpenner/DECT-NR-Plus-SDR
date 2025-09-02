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

#include <cstdint>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/ant.hpp"
#include "dectnrp/phy/agc/agc_config.hpp"
#include "dectnrp/phy/agc/roundrobin.hpp"

namespace dectnrp::phy::agc {

class agc_t {
    public:
        agc_t() = default;
        explicit agc_t(const agc_config_t agc_config_);

        static constexpr float OFDM_AMPLITUDE_FACTOR_MINUS_00dB = 1.0f;
        static constexpr float OFDM_AMPLITUDE_FACTOR_MINUS_03dB = 0.707945784f;
        static constexpr float OFDM_AMPLITUDE_FACTOR_MINUS_06dB = 0.501187233f;
        static constexpr float OFDM_AMPLITUDE_FACTOR_MINUS_10dB = 0.316227766f;
        static constexpr float OFDM_AMPLITUDE_FACTOR_MINUS_15dB = 0.177827941f;
        static constexpr float OFDM_AMPLITUDE_FACTOR_MINUS_20dB = 0.1f;

    protected:
        /// common AGC parameters for TX and RX
        agc_config_t agc_config;

        /// gain settings can be applied in the future, keeps track of when the pending values takes
        /// effect
        mutable int64_t power_ant_0dBFS_pending_time_64{common::adt::UNDEFINED_EARLY_64};

        roundrobin_t roundrobin;

        /**
         * \brief Determines the gain value closest to arbitrary_gain_step_dB adhering to the
         * limitations set by gain_step_dB_*.
         *
         * \param arbitrary_gain_step_dB any positive number
         * \return
         */
        float quantize_and_limit_gain_step_dB(float arbitrary_gain_step_dB);
        common::ant_t quantize_and_limit_gain_step_dB(const common::ant_t& arbitrary_gain_step_dB);

        static bool is_positive_multiple(const float inp, const float multiple);
};

}  // namespace dectnrp::phy::agc
