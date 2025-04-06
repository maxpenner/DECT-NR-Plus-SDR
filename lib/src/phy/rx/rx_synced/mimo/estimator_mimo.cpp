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

#include "dectnrp/phy/rx/rx_synced/mimo/estimator_mimo.hpp"

#include <volk/volk.h>

#include <bit>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/rx/rx_synced/rx_synced_param.hpp"

namespace dectnrp::phy {

estimator_mimo_t::estimator_mimo_t(const uint32_t N_RX_, const uint32_t N_TS_max_)
    : estimator_t(),
      N_RX(N_RX_),
      N_TS_max(N_TS_max_) {
    dectnrp_assert(0 < n_wideband_point, "must be positive");
    dectnrp_assert(n_wideband_point <= 8, "should not be more than 8");
    dectnrp_assert(n_wideband_point % 2 == 0, "must be even");

    for (std::size_t i = 0; i < N_TS_max; ++i) {
        stage_vec.push_back(srsran_vec_cf_malloc(N_RX * n_wideband_point));
    }

    stage_multiplication = srsran_vec_cf_malloc(N_RX * n_wideband_point);
}

estimator_mimo_t::~estimator_mimo_t() {
    free(stage_multiplication);

    for (auto elem : stage_vec) {
        free(elem);
    }
}

void estimator_mimo_t::process_stf(const channel_antennas_t& channel_antennas,
                                   const process_stf_meta_t& process_stf_meta) {
    /* The STF uses only transmit stream 0, thus we can't estimate the complete channel estimate if
     * more than one transmit stream is used.
     */

    mimo_report.fine_peak_time_64 = process_stf_meta.fine_peak_time_64;
}

void estimator_mimo_t::process_drs(const channel_antennas_t& channel_antennas,
                                   const process_drs_meta_t& process_drs_meta) {
    dectnrp_assert(N_RX == channel_antennas.size(), "incorrect size");

    /* Before running the actual MIMO algorithms, we put all common information required by all
     * algorithms onto the stage. We collect individual transmit streams of each RX antenna onto
     * a single stage. This operation does not take long since the spectrum is considered only
     * across n_wideband_point number of cells of the entire spectrum.
     */

    // transmit streams
    for (std::size_t ts = 0; ts <= process_drs_meta.TS_idx_last; ++ts) {
        // destination stage
        cf_t* const stage = stage_vec.at(ts);

        // RX antennas
        for (std::size_t rx = 0; rx < N_RX; ++rx) {
            // channel estimates of a single transmit stream
            const cf_t* tsptr = channel_antennas.at(rx)->get_chestim_drs_zf(ts);

            // cells
            for (std::size_t cell = 0; cell < n_wideband_point; ++cell) {
                stage[rx * n_wideband_point + cell] = tsptr[step_offset + cell * step_width];
            }
        }
    }

    // reset
    mimo_report = mimo_report_t();

    mode_single_spatial_stream_3_7();
    mode_multi_spatial_steam_2_4_6_8_9_11();
}

const mimo_report_t& estimator_mimo_t::get_mimo_report() const { return mimo_report; }

void estimator_mimo_t::reset_internal() {
    step_width = N_DRS_cells_b / n_wideband_point;
    step_offset = step_width / 2;

    dectnrp_assert(0 < step_width, "step_width too small");
}

void estimator_mimo_t::mode_single_spatial_stream_3_7() {
    /* MIMO modes 3 and 7 work regardless of the number of antennas at the opposite site. A single
     * transmit stream (index 0) if spread across the antenna streams.
     *
     *                                            A
     *      TS0_opposite = [Ch0, Ch1, Ch1, Ch3] * B * TS0
     *                                            C
     *                                            D
     *
     *                                            A
     *      TS0_opposite = [Ch0, Ch1, Ch1, Ch3] * B * TS0
     *      TS1_opposite   [Ch4, Ch5, Ch6, Ch7]   C
     *                                            D
     */

    // get reference to all available beamforming matrices, we transmit one TS
    const auto& W_mat = W.get_W(1, N_RX);

    // What is the index of the first W_mat without any zero elements?
    const uint32_t A = W.get_codebook_index_nonzero(1, N_RX);

    dectnrp_assert(A < W_mat.size(), "no matrices to use");

    // every beamforming matrix is tested, the one with the highest receive power is picked
    float power_max = -1000.0f;
    int32_t W_mat_best_idx = -1;

    // subset of W_mat
    for (std::size_t wm = A; wm < W_mat.size(); ++wm) {
        float power = 0.0f;

        const auto& W_mat_single = W_mat[wm];

        // RX antennas of opposite size
        for (std::size_t rxo = 0; rxo < N_eff_TX; ++rxo) {
            lv_32fc_t sum = lv_cmake(0.0f, 0.0f);

            for (std::size_t rx = 0; rx < N_RX; ++rx) {
                const std::size_t offset = rx * n_wideband_point;

                volk_32fc_s32fc_multiply2_32fc((lv_32fc_t*)stage_multiplication,
                                               (const lv_32fc_t*)&stage_vec.at(rxo)[offset],
                                               (const lv_32fc_t*)&W_mat_single.at(rx),
                                               n_wideband_point);

                lv_32fc_t sum_partial;

                volk_32fc_accumulator_s32fc(
                    &sum_partial, (const lv_32fc_t*)stage_multiplication, n_wideband_point);

                sum += sum_partial;
            }

            // determine total receiver power
            power += std::abs(sum);
        }

        // does it lead to more receive power?
        if (power_max < power) {
            power_max = power;
            W_mat_best_idx = wm;
        }
    }

    // add all information to the beamforming report
    mimo_report.tm_3_7_beamforming_idx = W_mat_best_idx;
}

void estimator_mimo_t::mode_multi_spatial_steam_2_4_6_8_9_11() {
    // ToDo
}

}  // namespace dectnrp::phy
