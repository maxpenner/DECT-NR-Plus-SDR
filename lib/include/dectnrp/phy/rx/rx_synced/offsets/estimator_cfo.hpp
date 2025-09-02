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

#include "dectnrp/common/complex.hpp"
#include "dectnrp/phy/rx/rx_synced/estimator/estimator.hpp"

namespace dectnrp::phy {

class estimator_cfo_t final : public estimator_t {
    public:
        estimator_cfo_t() = default;
        ~estimator_cfo_t() = default;

        estimator_cfo_t(const estimator_cfo_t&) = delete;
        estimator_cfo_t& operator=(const estimator_cfo_t&) = delete;
        estimator_cfo_t(estimator_cfo_t&&) = delete;
        estimator_cfo_t& operator=(estimator_cfo_t&&) = delete;

        /// can be called for every RX antenna
        virtual void process_stf(const channel_antennas_t& channel_antennas,
                                 const process_stf_meta_t& process_stf_meta) override final;

        virtual void process_drs(const channel_antennas_t& channel_antennas,
                                 const process_drs_meta_t& process_drs_meta) override final;

        /// we require the phase shift in time domain from sample to sample (s2s)
        [[nodiscard]] float get_residual_CFO_s2s_rad(const uint32_t N_DF_symbol_samples) const;

    private:
        virtual void reset_internal() override final;

        cf_t phase_rotation;

        void process_stf_phase_rotation(const cf_t* chestim_drs_zf);

        void process_drs_phase_rotation_model_lr(const cf_t* chestim_drs_zf);
};

}  // namespace dectnrp::phy
