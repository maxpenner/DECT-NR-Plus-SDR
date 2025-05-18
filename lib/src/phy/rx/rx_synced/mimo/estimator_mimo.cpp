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

#include <cmath>

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
    dectnrp_assert(0 < RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS, "must be positive");
    dectnrp_assert(RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS <= 14, "too large");
    dectnrp_assert(RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS % 2 == 0, "must be even");

    for (std::size_t i = 0; i < N_RX; ++i) {
        stage_rx_ts_vec.push_back(
            srsran_vec_cf_malloc(N_TS_max * RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS));
    }

    for (std::size_t i = 0; i < N_TS_max; ++i) {
        stage_rx_ts_transpose_vec.push_back(
            srsran_vec_cf_malloc(N_RX * RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS));
    }

    stage_multiplication =
        srsran_vec_cf_malloc(std::max(N_RX, N_TS_max) * RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS);
}

estimator_mimo_t::~estimator_mimo_t() {
    free(stage_multiplication);

    for (auto elem : stage_rx_ts_transpose_vec) {
        free(elem);
    }

    for (auto elem : stage_rx_ts_vec) {
        free(elem);
    }
}

void estimator_mimo_t::process_stf([[maybe_unused]] const channel_antennas_t& channel_antennas,
                                   [[maybe_unused]] const process_stf_meta_t& process_stf_meta) {
    /* The STF uses only transmit stream 0, thus we can't estimate the complete channel estimate if
     * more than one transmit stream is used.
     */

    // reset complete MIMO report
    mimo_report = mimo_report_t();
}

void estimator_mimo_t::process_drs(const channel_antennas_t& channel_antennas,
                                   const process_drs_meta_t& process_drs_meta) {
    dectnrp_assert(N_RX == channel_antennas.size(), "incorrect size");
    dectnrp_assert(process_drs_meta.TS_idx_first == 0, "incorrect number of transmit streams");
    dectnrp_assert(process_drs_meta.N_TS == process_drs_meta.TS_idx_last + 1,
                   "incorrect number of transmit streams");

    mimo_report.N_RX = N_RX;
    mimo_report.N_TS_other = process_drs_meta.N_TS;

    // extract selected cells of OFDM spectrum onto stages
    set_stages(channel_antennas, process_drs_meta);

    // optional beamforming matrix for other side to use
    if (process_drs_meta.N_TS == 1) {
        mimo_report.tm_3_7_beamforming_idx = 0;
    } else {
        mimo_report.tm_3_7_beamforming_idx = mode_single_spatial_stream_3_7(
            W, process_drs_meta.N_TS, N_RX, stage_rx_ts_vec, stage_multiplication);
    }

    // optional beamforming matrix for this receiver to use if channel is reciprocal
    if (N_RX == 1) {
        mimo_report.tm_3_7_beamforming_reciprocal_idx = 0;
    } else {
        mimo_report.tm_3_7_beamforming_reciprocal_idx = mode_single_spatial_stream_3_7(
            W, N_RX, process_drs_meta.N_TS, stage_rx_ts_transpose_vec, stage_multiplication);
    }
}

const mimo_report_t& estimator_mimo_t::get_mimo_report() const { return mimo_report; }

void estimator_mimo_t::reset_internal() {
    step_width = N_DRS_cells_b / RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS;
    step_offset = step_width / 2;

    dectnrp_assert(0 < step_width, "step_width too small");
}

void estimator_mimo_t::set_stages(const channel_antennas_t& channel_antennas,
                                  const process_drs_meta_t& process_drs_meta) {
    /* Before running the actual MIMO algorithms, we put all common information required by all
     * algorithms onto the stages. This operation does not take long since the spectrum is
     * considered only at RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS number of cells of the entire
     * spectrum.
     */

    // RX antennas
    for (std::size_t rx = 0; rx < N_RX; ++rx) {
        // destination stage
        cf_t* const stage = stage_rx_ts_vec.at(rx);

        // transmit streams
        for (std::size_t ts = 0; ts <= process_drs_meta.TS_idx_last; ++ts) {
            // channel estimates of a single transmit stream
            const cf_t* tsptr = channel_antennas.at(rx)->get_chestim_drs_zf(ts);

            // cells
            for (std::size_t cell = 0; cell < RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS; ++cell) {
                stage[ts * RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS + cell] =
                    tsptr[step_offset + cell * step_width];
            }
        }
    }

    // transmit streams
    for (std::size_t ts = 0; ts <= process_drs_meta.TS_idx_last; ++ts) {
        // destination stage
        cf_t* const stage = stage_rx_ts_transpose_vec.at(ts);

        // RX antennas
        for (std::size_t rx = 0; rx < N_RX; ++rx) {
            // destination stage
            const cf_t* stage_src = stage_rx_ts_vec.at(rx);

            srsran_vec_cf_copy(&stage[rx * RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS],
                               &stage_src[ts * RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS],
                               RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS);
        }
    }
}

uint32_t estimator_mimo_t::mode_single_spatial_stream_3_7(const section3::W_t& W,
                                                          const uint32_t N_TX_virt,
                                                          const uint32_t N_RX_virt,
                                                          const std::vector<cf_t*>& stage,
                                                          cf_t* stage_multiplication) {
    dectnrp_assert(1 < N_TX_virt, "single TX antenna does not allow beamforming");

    // get reference to all available beamforming matrices for a single transmit stream
    const auto& W_mat = W.get_W(1, N_TX_virt);
    const auto& scaling_factor = W.get_scaling_factor(1, N_TX_virt);

#ifdef RX_SYNCED_PARAM_MIMO_USE_ALL_W_MATRICES_OR_ONLY_NON_ZERO
    const uint32_t A = 0;
#else
    // What is the index of the first W_mat without any zero elements?
    const uint32_t A = W.get_codebook_index_nonzero(1, N_TX_virt);
#endif

    dectnrp_assert(A < W_mat.size(), "no matrices to use");

#if RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_HIGHEST_MIN_RX_POWER
    float power_outer = -1.0e6;
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MAX_RX_POWER
    float power_outer = -1.0e6;
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MIN_SPREAD_RX_POWER
    float power_outer = 1.0e6;
#else
#error "undefined mode 3 and 7 metric"
#endif

    int32_t ret = -1;

    for (std::size_t wm = A; wm < W_mat.size(); ++wm) {
        // single beamforming matrix to test
        const auto& W_mat_single = W_mat[wm];

#if RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_HIGHEST_MIN_RX_POWER
        float power_inner = 1.0e6;
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MAX_RX_POWER
        float power_inner = 0.0f;
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MIN_SPREAD_RX_POWER
        float power_max = -1.0e6;
        float power_min = 1.0e6;
        float power_inner = 0.0f;
#else
#error "undefined mode 3 and 7 metric"
#endif

        // RX antennas of opposite size
        for (std::size_t rx = 0; rx < N_RX_virt; ++rx) {
            // sum of power across all TS for a single RX antenna
            lv_32fc_t sum = lv_cmake(0.0f, 0.0f);

            for (std::size_t tx = 0; tx < N_TX_virt; ++tx) {
                volk_32fc_s32fc_multiply2_32fc(
                    (lv_32fc_t*)stage_multiplication,
                    (const lv_32fc_t*)&stage.at(rx)[tx * RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS],
                    (const lv_32fc_t*)&W_mat_single.at(tx),
                    RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS);

                lv_32fc_t sum_partial;

                volk_32fc_accumulator_s32fc(&sum_partial,
                                            (const lv_32fc_t*)stage_multiplication,
                                            RX_SYNCED_PARAM_MIMO_N_WIDEBAND_CELLS);

                sum += sum_partial;
            }

#if RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_HIGHEST_MIN_RX_POWER
            if (std::abs(sum) < power_inner) {
                power_inner = std::abs(sum);
            }
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MAX_RX_POWER
            power_inner += std::abs(sum);
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MIN_SPREAD_RX_POWER
            power_min = std::min(power_min, std::abs(sum));
            power_max = std::max(power_max, std::abs(sum));
            power_inner = power_max - power_min;
#else
#error "undefined mode 3 and 7 metric"
#endif
        }

        power_inner *= scaling_factor[wm];

#if RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_HIGHEST_MIN_RX_POWER
        if (power_outer < power_inner) {
            power_outer = power_inner;
            ret = wm;
        }
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MAX_RX_POWER
        if (power_outer < power_inner) {
            power_outer = power_inner;
            ret = wm;
        }
#elif RX_SYNCED_PARAM_MODE_3_7_METRIC == RX_SYNCED_PARAM_MODE_3_7_METRIC_MIN_SPREAD_RX_POWER
        if (power_outer > power_inner) {
            power_outer = power_inner;
            ret = wm;
        }
#else
#error "undefined mode 3 and 7 metric"
#endif
    }

    dectnrp_assert(0 <= ret, "invalid matrix index");

    return ret;
}

}  // namespace dectnrp::phy
