/*
 * Copyright 2023-2024 Maxim Penner, Mattes Wassmann
 * Copyright 2025-2025 Maxim Penner
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

#include "dectnrp/phy/rx/sync/coarse_peak_f_domain.hpp"

#include <volk/volk.h>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/sections_part3/transmission_packet_structure.hpp"

namespace dectnrp::phy {

coarse_peak_f_domain_t::coarse_peak_f_domain_t(const std::vector<cf_t*> localbuffer_,
                                               const uint32_t u_max_,
                                               const uint32_t b_max_,
                                               const uint32_t nof_antennas_limited_,
                                               const uint32_t bos_fac_)
    : localbuffer(localbuffer_),
      b_max(b_max_),
      nof_antennas_limited(nof_antennas_limited_),
      N_samples_STF_CP_only_os(
          sp3::transmission_packet_structure::get_N_samples_STF_CP_only(u_max_, b_max) * bos_fac_ /
          b_max) {
    dectnrp_assert(bos_fac_ / b_max >= 1, "oversampling must be at least 1");

    get_ofdm(ofdm, bos_fac_ * constants::N_b_DFT_min_u_b);

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        fft_stage.push_back(srsran_vec_cf_malloc(ofdm.N_b_DFT_os));
    }
}

coarse_peak_f_domain_t::~coarse_peak_f_domain_t() {
    free_ofdm(ofdm);

    for (auto elem : fft_stage) {
        free(elem);
    }
}

std::pair<uint32_t, float> coarse_peak_f_domain_t::process(
    [[maybe_unused]] const uint32_t coarse_peak_time_local) {
    // we only mix and FFT if required
#if defined(RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_FIND_BETA_THRESHOLD_DB_OR_ASSUME_MAX_OF_RDC) || \
    defined(RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_FIND_INTEGER_CFO_SEARCH_RANGE_OR_ASSUME_ZERO)

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        // correct fractional CFO starting at the coarse peak
        // ToDo

        // FFT
        dft::single_symbol_rx_ofdm_zero_copy(ofdm,
                                             &localbuffer[ant_idx][coarse_peak_time_local],
                                             fft_stage[ant_idx],
                                             N_samples_STF_CP_only_os);

        // FFT shift
        dft::mem_mirror(fft_stage[ant_idx], ofdm.N_b_DFT_os);
    }
#endif

    return std::make_pair(estimate_beta(), estimate_integer_cfo());
}

uint32_t coarse_peak_f_domain_t::estimate_beta() const {
#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_FIND_BETA_THRESHOLD_DB_OR_ASSUME_MAX_OF_RDC
    // if there is nothing to estimate, return immediately
    if (b_max == 1) {
        return b_max;
    }

    // we start by assuming beta is 1
    uint32_t b_estim = 1;

    // with beta being 1, we have 32 subcarrier on the left and 32 on the right
    uint32_t n_subc_sideband = constants::N_b_DFT_min_u_b / 2;

    // what are the indices of the subcarriers, where the left and right sidebands start?
    uint32_t idx_left_sideband = ofdm.N_b_DFT_os / 2 - n_subc_sideband;
    uint32_t idx_right_sideband = ofdm.N_b_DFT_os / 2;

    // calculates combined sideband power
    auto calc_power_sidebands = [&]() {
        // accumulator across antennas
        float power = 0.0f;

        for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
            lv_32fc_t sideband_power;

            // left band
            volk_32fc_x2_conjugate_dot_prod_32fc(
                &sideband_power,
                (const lv_32fc_t*)&fft_stage[ant_idx][idx_left_sideband],
                (const lv_32fc_t*)&fft_stage[ant_idx][idx_left_sideband],
                n_subc_sideband);
            power += sideband_power.real();

            // right band
            volk_32fc_x2_conjugate_dot_prod_32fc(
                &sideband_power,
                (const lv_32fc_t*)&fft_stage[ant_idx][idx_right_sideband],
                (const lv_32fc_t*)&fft_stage[ant_idx][idx_right_sideband],
                n_subc_sideband);
            power += sideband_power.real();
        }

        return power;
    };

    // initialize power for beta=1 by reusing the lambda
    float central_power = calc_power_sidebands();

    // convert threshold from dB
    const float threshold = dectnrp::common::adt::db2mag(
        RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_FIND_BETA_THRESHOLD_DB_OR_ASSUME_MAX_OF_RDC);

    /* Our current best guess for beta is b_estim. We will now keep testing the next larger beta
     * value until we either have tested all, or until we believe to have found the correct of beta.
     */
    for (uint32_t b_idx = 1; b_estim <= b_max; ++b_idx) {
        // next beta to best
        const uint32_t b_estim_test = sp3::phyres::b_idx2b[b_idx];

        dectnrp_assert(b_estim_test > b_estim, "tested beta out-of-order");

        // update the number of subcarriers in the sidebands
        n_subc_sideband = (b_estim_test - b_estim) * constants::N_b_DFT_min_u_b / 2;

        dectnrp_assert(idx_left_sideband >= n_subc_sideband,
                       "not enough subcarriers in left sideband");
        dectnrp_assert(idx_right_sideband + n_subc_sideband <= ofdm.N_b_DFT_os,
                       "not enough subcarriers in right sideband");

        // move indices to the left and right
        idx_left_sideband -= n_subc_sideband;
        idx_right_sideband += n_subc_sideband;

        // calculate combined new sideband power
        const float side_band_power = calc_power_sidebands();

        // normalize central/sideband power with the number of subcarriers they contain
        const float central_power_norm =
            central_power / static_cast<float>(b_estim * constants::N_b_DFT_min_u_b);
        const float side_band_power_norm =
            side_band_power / static_cast<float>(n_subc_sideband * 2);

        // is there still a lot of power in the sidebands compared to the central power?
        if (side_band_power_norm / central_power_norm > threshold) {
            // our new best guess
            b_estim = b_estim_test;

            // sideband power now become part of the central power
            central_power += side_band_power;
        } else {
            // power from center to sidebands dropped sharply, beta found, abort prematurely
            break;
        }
    }

    return b_estim;
#else
    return b_max;
#endif
}

float coarse_peak_f_domain_t::estimate_integer_cfo() const {
#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_FIND_INTEGER_CFO_SEARCH_RANGE_OR_ASSUME_ZERO
#error "integer CFO not implemented yet"
#else
    return 0.0f;
#endif
}

}  // namespace dectnrp::phy
