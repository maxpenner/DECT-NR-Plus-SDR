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

#include "dectnrp/phy/rx/rx_synced/snr/estimator_snr.hpp"

#include <volk/volk.h>

#include <bit>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/rx/rx_synced/rx_synced_param.hpp"
#include "dectnrp/sections_part3/drs.hpp"

namespace dectnrp::phy {

estimator_snr_t::estimator_snr_t(const uint32_t b_max)
    : estimator_t(),
      subtraction_stage(srsran_vec_cf_malloc(section3::drs_t::get_nof_drs_subc(b_max) * 2)) {
    dectnrp_assert(std::has_single_bit(RX_SYNCED_PARAM_SNR_BASED_ON_DRS_N_TS_MAX) &&
                       RX_SYNCED_PARAM_SNR_BASED_ON_DRS_N_TS_MAX <= 8,
                   "ill-defined");
}

estimator_snr_t::~estimator_snr_t() { free(subtraction_stage); }

void estimator_snr_t::process_stf(const channel_antennas_t& channel_antennas) {
    dectnrp_assert(snr_acc.S_plus_N_cnt == 0, "no reset");
    dectnrp_assert(snr_acc.N_cnt == 0, "no reset");

    // go over each RX antenna
    for (std::size_t ant_idx = 0; ant_idx < channel_antennas.size(); ++ant_idx) {
        // by convention is assumed the channel estimation has been written to transmit stream 0
        snr_acc += process_stf_or_drs_packed(channel_antennas[ant_idx]->get_chestim_drs_zf(0),
                                             N_STF_cells_b);
    }

    /* According to 6.3.5 OFDM signal generation, the STF cells are amplitude boosted, thus the SNR
     * for STF cells is higher than for DRS cells. The boost is removed by overwriting with a
     * smaller receive power.
     */
    const float S = snr_acc.S_plus_N_sum - snr_acc.N_sum;
    snr_acc.S_plus_N_sum = S / 4.0f + snr_acc.N_sum;
}

void estimator_snr_t::process_drs(const channel_antennas_t& channel_antennas,
                                  const process_drs_meta_t& process_drs_meta) {
    const size_t A = std::min(process_drs_meta.TS_idx_last,
                              std::size_t{RX_SYNCED_PARAM_SNR_BASED_ON_DRS_N_TS_MAX - 1});

    // go over each RX antenna
    for (uint32_t ant_idx = 0; ant_idx < channel_antennas.size(); ++ant_idx) {
        // go over all transmit streams
        for (uint32_t ts_idx = process_drs_meta.TS_idx_first; ts_idx <= A; ++ts_idx) {
            // calculate SNR and add to accumulator
            snr_acc += process_stf_or_drs_packed(
                channel_antennas[ant_idx]->get_chestim_drs_zf(ts_idx), N_DRS_cells_b);
        }
    }
}

float estimator_snr_t::get_current_snr_dB_estimation() const {
    if (snr_acc.S_plus_N_sum <= 0.0f || snr_acc.N_sum <= 0.0f) {
        return 0.0f;
    }

    const float S_avg =
        (snr_acc.S_plus_N_sum - snr_acc.N_sum) / static_cast<float>(snr_acc.S_plus_N_cnt);
    const float N_avg = snr_acc.N_sum / static_cast<float>(snr_acc.N_cnt);

    return common::adt::pow2db(S_avg / N_avg);
}

void estimator_snr_t::reset_internal() {
    snr_acc.S_plus_N_sum = 0.0f;
    snr_acc.S_plus_N_cnt = 0;

    snr_acc.N_sum = 0.0f;
    snr_acc.N_cnt = 0;
}

estimator_snr_t::snr_acc_t estimator_snr_t::process_stf_or_drs_packed(const cf_t* chestim_drs_zf,
                                                                      const uint32_t nof_pilots) {
    // init for this vector
    snr_acc_t ret;

    // noise power is extracted by comparing neighbouring pilots
    const uint32_t nof_pairs = nof_pilots - 1;

    // S symbol power

    // equation (16) from "SNR Estimation Algorithm Based on the Preamble for OFDM Systems
    // in Frequency Selective Channels"
    lv_32fc_t tmp;
    volk_32fc_x2_conjugate_dot_prod_32fc(
        &tmp, (const lv_32fc_t*)chestim_drs_zf, (const lv_32fc_t*)chestim_drs_zf, nof_pilots);

    ret.S_plus_N_sum += tmp.real();
    ret.S_plus_N_cnt += nof_pilots;

    // N noise power

    /* equation (9) from "SNR Estimation Algorithm Based on the Preamble for OFDM Systems in
     * Frequency Selective Channels"
     *
     * pairwise subtraction, function defined for float, thus nof_pairs*2
     */
    volk_32f_x2_subtract_32f((float*)subtraction_stage,
                             (float*)chestim_drs_zf,
                             (float*)&chestim_drs_zf[1],
                             nof_pairs * 2);

    // self conjugate dot product yielding norm squared
    volk_32fc_x2_conjugate_dot_prod_32fc(
        &tmp, (const lv_32fc_t*)subtraction_stage, (const lv_32fc_t*)subtraction_stage, nof_pairs);

    // normalize, according to equation (9) we collect twice the noise power
    ret.N_sum += tmp.real() / 2.0f;
    ret.N_cnt += nof_pairs;

    return ret;
}

}  // namespace dectnrp::phy
