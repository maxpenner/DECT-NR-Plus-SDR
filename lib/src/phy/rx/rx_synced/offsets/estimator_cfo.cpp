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

#include "dectnrp/phy/rx/rx_synced/offsets/estimator_cfo.hpp"

#include <complex>

#include "dectnrp/common/complex.hpp"

namespace dectnrp::phy {

void estimator_cfo_t::process_stf(const channel_antennas_t& channel_antennas,
                                  const process_stf_meta_t& process_stf_meta) {}

void estimator_cfo_t::process_drs(const channel_antennas_t& channel_antennas,
                                  const process_drs_meta_t& process_drs_meta) {
    /* Common phase error (CPE) is symbol-dependent, but not subcarrier-dependent (for small
     * residual CFOs, which is assumed to be true). Here, we compare the phase rotation from OFDM
     * symbol to OFDM symbol for subcarriers that are close in frequency domain. In mode lr
     * subcarriers are interlaced.
     */

    // this new estimation depends only on current processing stage
    phase_rotation = cf_t{0.0f, 0.0f};

    // go over each RX antenna's OFDM symbol in frequency domain
    for (uint32_t ant_idx = 0; ant_idx < channel_antennas.size(); ++ant_idx) {
        // go over each transmit stream
        for (uint32_t ts_idx = process_drs_meta.TS_idx_first;
             ts_idx <= process_drs_meta.TS_idx_last;
             ++ts_idx) {
            // DRS channel estimation for residual CFO estimation
            process_drs_phase_rotation_model_lr(
                channel_antennas[ant_idx]->get_chestim_drs_zf(ts_idx));
        }
    }
}

// we require the phase shift in time domain from sample to sample (s2s)
float estimator_cfo_t::get_residual_CFO_s2s_rad(const uint32_t N_DF_symbol_samples) const {
    return std::arg(std::complex<float>(phase_rotation)) / static_cast<float>(N_DF_symbol_samples);
}

void estimator_cfo_t::reset_internal() {}

void estimator_cfo_t::process_stf_phase_rotation(const cf_t* chestim_drs_zf) {
    //

    //
}

void estimator_cfo_t::process_drs_phase_rotation_model_lr(const cf_t* chestim_drs_zf) {
    /* Common phase error (CPE) is symbol-dependent, but not subcarrier-dependent (for small
     * residual CFOs, which is assumed to be true). Here, we compare the phase rotation from OFDM
     * symbol to OFDM symbol for subcarriers that are close in frequency domain. In mode lr
     * subcarriers are interlaced.
     */
    for (uint32_t i = 0; i < N_DRS_cells_b - 1; ++i) {
        phase_rotation += (chestim_drs_zf[i] * conjj(chestim_drs_zf[i + 1]));
    }
}

}  // namespace dectnrp::phy
