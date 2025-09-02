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

#include <volk/volk.h>

#include <cstdint>
#include <vector>

#include "dectnrp/common/complex.hpp"
#include "dectnrp/phy/rx/rx_synced/estimator/estimator.hpp"

namespace dectnrp::phy {

class estimator_sto_t final : public estimator_t {
    public:
        estimator_sto_t(const uint32_t b_max);
        ~estimator_sto_t();

        estimator_sto_t() = delete;
        estimator_sto_t(const estimator_sto_t&) = delete;
        estimator_sto_t& operator=(const estimator_sto_t&) = delete;
        estimator_sto_t(estimator_sto_t&&) = delete;
        estimator_sto_t& operator=(estimator_sto_t&&) = delete;

        virtual void process_stf(const channel_antennas_t& channel_antennas,
                                 const process_stf_meta_t& process_stf_meta) override final;

        virtual void process_drs(const channel_antennas_t& channel_antennas,
                                 const process_drs_meta_t& process_drs_meta) override final;

        void apply_full_phase_rotation(std::vector<cf_t*>& ofdm_symbol_now);

        [[nodiscard]] float get_fractional_sto_in_samples(const uint32_t N_b_DFT_os) const;

    private:
        virtual void reset_internal() override final;

        cf_t* stage;

        float phase_increment_rad;

        lv_32fc_t phase_increment;
        lv_32fc_t phase_start;

        [[nodiscard]] float process_stf_phase_rotation(const cf_t* chestim_drs_zf);
        [[nodiscard]] float process_drs_phase_rotation(const cf_t* chestim_drs_zf);

        void convert_to_phasors();
};

}  // namespace dectnrp::phy
