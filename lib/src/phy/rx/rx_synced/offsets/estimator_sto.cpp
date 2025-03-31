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

#include "dectnrp/phy/rx/rx_synced/offsets/estimator_sto.hpp"

#include <bit>
#include <cmath>
#include <complex>
#include <numbers>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/rx/rx_synced/rx_synced_param.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::phy {

estimator_sto_t::estimator_sto_t(const uint32_t b_max)
    : estimator_t(),
      stage(srsran_vec_cf_malloc(b_max * constants::N_STF_cells_b_1)) {
    dectnrp_assert(std::has_single_bit(RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS_N_TS_MAX) &&
                       RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS_N_TS_MAX <= 8,
                   "ill-defined");
}

estimator_sto_t::~estimator_sto_t() { free(stage); }

void estimator_sto_t::process_stf(const channel_antennas_t& channel_antennas) {
    dectnrp_assert(phase_increment_rad == 0.0f, "must be 0.0f for STF");

    // go over each RX antenna
    for (std::size_t ant_idx = 0; ant_idx < channel_antennas.size(); ++ant_idx) {
        // by convention is assumed the channel estimation has been written to transmit stream 0
        phase_increment_rad +=
            process_stf_phase_rotation(channel_antennas[ant_idx]->get_chestim_drs_zf(0));
    }

    // average phase rotation
    phase_increment_rad /= static_cast<float>(channel_antennas.size());

    convert_to_phasors();
}

void estimator_sto_t::process_drs(const channel_antennas_t& channel_antennas,
                                  const process_drs_meta_t& process_drs_meta) {
    float phase_increment_rad_local = 0.0f;

    const size_t A = std::min(process_drs_meta.TS_idx_last,
                              std::size_t{RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS_N_TS_MAX - 1});

    std::size_t cnt = 0;

    // go over each RX antenna's OFDM symbol in frequency domain
    for (uint32_t ant_idx = 0; ant_idx < channel_antennas.size(); ++ant_idx) {
        // go over each transmit stream
        for (uint32_t ts_idx = process_drs_meta.TS_idx_first; ts_idx <= A; ++ts_idx) {
            // DRS channel estimation for residual CFO estimation
            phase_increment_rad_local +=
                process_drs_phase_rotation(channel_antennas[ant_idx]->get_chestim_drs_zf(ts_idx));

            ++cnt;
        }
    }

    // can happen if the maximum index is limited to 0, but there are more transmit streams
    if (cnt == 0) {
        return;
    }

    // average phase rotation
    phase_increment_rad_local /= static_cast<float>(cnt);

    // add residual to currently used rotation
    phase_increment_rad += phase_increment_rad_local;

    convert_to_phasors();
}

void estimator_sto_t::apply_full_phase_rotation(std::vector<cf_t*>& ofdm_symbol_now) {
    // go over each RX antenna
    for (std::size_t ant_idx = 0; ant_idx < ofdm_symbol_now.size(); ++ant_idx) {
        // we need a local copy since we want to derotate from the same angles
        lv_32fc_t phase_start_local = phase_start;

        volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t*)ofdm_symbol_now.at(ant_idx),
                                         (const lv_32fc_t*)ofdm_symbol_now.at(ant_idx),
                                         &phase_increment,
                                         &phase_start_local,
                                         N_b_OCC_plus_DC);
    }
}

float estimator_sto_t::get_fractional_sto_in_samples(const uint32_t N_b_DFT_os) const {
    const float A =
        std::arg(std::complex<float>(phase_increment)) / 2.0 / std::numbers::pi_v<float>;
    return A * static_cast<float>(N_b_DFT_os);
}

void estimator_sto_t::reset_internal() {
    phase_increment_rad = 0.0f;
    convert_to_phasors();
}

float estimator_sto_t::process_stf_phase_rotation(const cf_t* chestim_drs_zf) {
    // create pairwise product between neighbouring values
    volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t*)stage,
                                         (const lv_32fc_t*)chestim_drs_zf,
                                         (const lv_32fc_t*)&chestim_drs_zf[1],
                                         N_STF_cells_b - 1);

    /* In the center, around the DC carrier, the phase rotation is measured across 8 subcarrier
     * instead of 4, so we rotate the complex phasor by half its angle in the opposite direction.
     */
    const size_t center_idx = N_STF_cells_b / 2 - 1;
    const lv_32fc_t A = (lv_32fc_t)stage[center_idx];
    const float A_arg = atan2(A.imag(), A.real());
    const lv_32fc_t A_derot = A * lv_cmake(std::cos(-A_arg / 2.0f), std::sin(-A_arg / 2.0f));
    stage[center_idx] = cf_t{A_derot.real(), A_derot.imag()};

    // get sum of all values
    lv_32fc_t B;
    volk_32fc_accumulator_s32fc_a(&B, (const lv_32fc_t*)stage, N_STF_cells_b - 1);

    // phase rotation from subcarrier to subcarrier
    return atan2(B.imag(), B.real()) / static_cast<float>(constants::N_STF_cells_separation);
}

// helpers for residual STO based on DRS
float estimator_sto_t::process_drs_phase_rotation(const cf_t* chestim_drs_zf) {
    // create pairwise product between neighbouring values
    volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t*)stage,
                                         (const lv_32fc_t*)chestim_drs_zf,
                                         (const lv_32fc_t*)&chestim_drs_zf[1],
                                         N_DRS_cells_b - 1);

    // get sum of all values
    lv_32fc_t B;
    volk_32fc_accumulator_s32fc_a(&B, (const lv_32fc_t*)stage, N_DRS_cells_b - 1);

    // phase rotation from subcarrier to subcarrier
    return atan2(B.imag(), B.real()) / static_cast<float>(constants::N_STF_cells_separation);
}

void estimator_sto_t::convert_to_phasors() {
    phase_increment = lv_cmake(std::cos(phase_increment_rad), std::sin(phase_increment_rad));

    // asymmetrical derotation such that at DC derotation is zero
    const float phase_start_rad =
        -phase_increment_rad * static_cast<float>((N_b_OCC_plus_DC - 1) / 2);

    phase_start = lv_cmake(std::cos(phase_start_rad), std::sin(phase_start_rad));
}

}  // namespace dectnrp::phy
