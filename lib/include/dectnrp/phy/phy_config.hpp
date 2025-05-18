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

#include <array>
#include <string>

#include "dectnrp/common/layer/layer_config.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/phy/worker_pool_config.hpp"

/* Default LLR bit width is 16 bit, which is also stable. To enable 8 bit processing, a function in
 * the srsRAN installation has to be changed.
 *
 *  1. Go to file https://github.com/srsran/srsRAN/blob/master/lib/src/phy/fec/turbo/rm_turbo.c
 *  2. Go to function srsran_rm_turbo_rx_lut_8bit().
 *  3. Deactivate the code between #if SRSRAN_TDEC_EXPECT_INPUT_SB == 1 ... #else and activate line
 *      uint16_t* deinter = deinterleaver[cb_idx][rv_idx];
 *  4. Recompile and reinstall srsRAN.
 */
#define PHY_LLR_BIT_WIDTH 16
#define PHY_D_RX_DATA_TYPE int16_t
#define PHY_D_RX_DATA_TYPE_SIZE sizeof(PHY_D_RX_DATA_TYPE)

namespace dectnrp::phy {

class phy_config_t final : public common::layer_config_t<worker_pool_config_t> {
    public:
        phy_config_t() = default;
        explicit phy_config_t(const std::string directory);

        /// check if conversion from oversampled DECTNRP sample rate to hardware sample rate is
        /// possible
        static resampler_param_t get_resampler_param_verified(
            const uint32_t hw_samp_rate,
            const uint32_t dect_samp_rate_os,
            const bool enforce_dectnrp_samp_rate_by_resampling);

    private:
        /// list of acceptable sample rates for both simulation and hardware
        static const std::array<resampler_param_t, 28> resampler_param_verified;
};

}  // namespace dectnrp::phy
