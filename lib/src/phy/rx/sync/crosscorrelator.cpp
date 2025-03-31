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

#include "dectnrp/phy/rx/sync/crosscorrelator.hpp"

#include <volk/volk.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/phy/mix/mixer.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::phy {

crosscorrelator_t::crosscorrelator_t(const std::vector<cf_t*> localbuffer_,
                                     const uint32_t u_max_,
                                     const uint32_t b_max_,
                                     const uint32_t os_min_,
                                     const uint32_t nof_antennas_,
                                     const uint32_t nof_antennas_limited_,
                                     const resampler_param_t resampler_param_)
    : correlator_t(localbuffer_),
      nof_antennas(nof_antennas_),
      nof_antennas_limited(nof_antennas_limited_),
      resampler_param(resampler_param_),

      search_length_l(RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_LEFT_SAMPLES * b_max_ * os_min_ *
                      resampler_param.L / resampler_param.M),
      search_length_r(RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_RIGHT_SAMPLES * b_max_ * os_min_ *
                      resampler_param.L / resampler_param.M),
      search_length(search_length_l + 1 + search_length_r),

      stf_template(
          std::make_unique<stf_template_t>(u_max_, b_max_, os_min_, nof_antennas, resampler_param)),

      mixer_stage_len(search_length_l + stf_template->stf_bos_rs_length_samples + search_length_r) {
    dectnrp_assert(nof_antennas_limited <= nof_antennas, "too many antennas");

    // init mixing stage for every antenna
    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        mixer_stage.push_back(srsran_vec_cf_malloc(mixer_stage_len));
    }

    xcorr_stage = srsran_vec_cf_malloc(search_length);

    peak_vec.resize(nof_antennas_limited, peak_t(nof_antennas_limited));
}

crosscorrelator_t::~crosscorrelator_t() {
    for (auto& elem : mixer_stage) {
        free(elem);
    }

    free(xcorr_stage);
}

void crosscorrelator_t::set_initial_state() { localbuffer_cnt_r = 0; }

uint32_t crosscorrelator_t::get_nof_samples_required() const {
    return search_length + stf_template->stf_bos_rs_length_samples - 1;
}

bool crosscorrelator_t::search_by_correlation(const uint32_t localbuffer_cnt_w,
                                              sync_report_t& sync_report) {
    dectnrp_assert(get_nof_samples_required() <= localbuffer_cnt_w,
                   "number of samples must be larger");

#ifdef RX_SYNC_PARAM_CROSSCORRELATOR_CFO_PRECORRECTION
    /* Fractional CFO in the sync_report_t was determined after resampling to a DECTNRP sample
     * rate. Here, we transform it back to the sample rate of the hardware. Therefore, the CFO must
     * either stay the same of become smaller because the hardware is sampling faster and the
     * sample-to-sample rotation become smaller.
     */
    const float cfo_hw_rad = (sync_report.cfo_fractional_rad + sync_report.cfo_integer_rad) *
                             static_cast<float>(resampler_param.M) /
                             static_cast<float>(resampler_param.L);

    // setup mixer for CFO correction
    mixer.set_phase(0.0f);
    mixer.set_phase_increment(cfo_hw_rad);
#else
    // setup mixer for CFO correction
    mixer.set_phase(0.0f);
    mixer.set_phase_increment(0.0f);
#endif

    // derotate all samples required for fine search
    mixer.mix_phase_continuous_offset(
        localbuffer, localbuffer_cnt_r, mixer_stage, 0, mixer_stage_len);

    run_fine_search(sync_report);

    return true;
}

void crosscorrelator_t::run_fine_search(sync_report_t& sync_report) {
    dectnrp_assert(sync_report.coarse_peak_array.get_nof_antennas() == nof_antennas_limited,
                   "not the same number of antennas");

    /* The container sync_report.coarse_peak_array is an array of fixed size with as many float
     * elements as DECTNRP supports antennas, i.e. typically 8 values even if the radio device class
     * uses less antennas. Each float represents whether a valid coarse peak metric was found at the
     * given antenna index. In coarse_peak_ant_idx_vec we collect only the indices of the
     * antennas we want process, and write them to the front of the array.
     */
    std::array<uint8_t, limits::dectnrp_max_nof_antennas> coarse_peak_ant_idx_vec{};

    // number of valid coarse peaks that are used for processing
    std::size_t nof_coarse_peak_processable = 0;

#ifdef RX_SYNC_PARAM_CROSSCORRELATOR_ALL_ANTENNAS_OR_ONLY_STRONGEST
    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        if (0.0f < sync_report.coarse_peak_array.at(ant_idx)) {
            coarse_peak_ant_idx_vec.at(nof_coarse_peak_processable++) = ant_idx;
        }
    }

    dectnrp_assert(0 < nof_coarse_peak_processable, "no antennas to process");
    dectnrp_assert(
        nof_coarse_peak_processable == sync_report.coarse_peak_array.get_nof_larger(0.0f),
        "not the same number of antennas");
#else
    // only the one antenna with the strongest peak is processed
    nof_coarse_peak_processable = 1;

    coarse_peak_ant_idx_vec.at(0) = sync_report.coarse_peak_array.get_index_of_max();

    dectnrp_assert(0.0f < sync_report.coarse_peak_array.at(coarse_peak_ant_idx_vec.at(0)),
                   "coarse peak zero for maximum");
#endif

    // load all STF templates
    const std::vector<cf_t*>& templates = stf_template->get_stf_time_domain(sync_report.b);

    dectnrp_assert(templates.size() == section3::phyres::N_TS_2_nof_STF_templates_vec[nof_antennas],
                   "incorrect number of STF templates");

    // conduct fine search only for antennas that have a valid coarse peak
    for (std::size_t i = 0; i < nof_coarse_peak_processable; ++i) {
        const uint32_t ant_idx = coarse_peak_ant_idx_vec[i];

        dectnrp_assert(0.0f < sync_report.coarse_peak_array.at(ant_idx), "coarse peak is zero");

        /* For every valid RX antenna, perform a crosscorrelation with every STF template to find
         * best fitting peak and most likely N_eff_TX.
         */
        for (uint32_t N_eff_TX_idx = 0; N_eff_TX_idx < templates.size(); ++N_eff_TX_idx) {
            // has to be reset when template changes
            cf_t* xcorr_stage_cpy = xcorr_stage;

            // readability
            const cf_t* iq_a = mixer_stage[ant_idx];
            const cf_t* iq_b = templates[N_eff_TX_idx];

            // go over the entire search range
            for (uint32_t j = 0; j < search_length; ++j) {
                volk_32fc_x2_conjugate_dot_prod_32fc(
                    (lv_32fc_t*)xcorr_stage_cpy++,
                    (const lv_32fc_t*)iq_a++,
                    (const lv_32fc_t*)iq_b,
                    stf_template->stf_bos_rs_length_effective_samples);
            }

            // find index of largest magnitude=metric
            uint32_t idx;
            volk_32fc_index_max_32u(&idx, (lv_32fc_t*)xcorr_stage, search_length);

            // extract the metric
            const float metric = std::abs(std::complex<float>(xcorr_stage[idx]));

            // idx is relative to xcorr_stage, make if relative to localbuffer
            idx += localbuffer_cnt_r;

            // save for later evaluation
            peak_vec[ant_idx].set_metric_index(metric, idx, N_eff_TX_idx);
        }
    }

    /* At this point, we are done with the crosscorrelation. We can have at most 1x1 peaks, 2x2
     * peaks, 4x3 peaks or 8x4 peaks (nof RX antennas x nof templates).
     *
     * Example: For a system with four antennas, we have a total of 4 x 3 peaks. 4 RX antennas, each
     * tested against 3 templates (N_eff_TX = 1, 2 or 4).
     *
     * RX antenna 0: peak for N_eff_TX=1 | peak for N_eff_TX=2 | peak for N_eff_TX=4
     * RX antenna 1: peak for N_eff_TX=1 | peak for N_eff_TX=2 | peak for N_eff_TX=4
     * RX antenna 2: peak for N_eff_TX=1 | peak for N_eff_TX=2 | peak for N_eff_TX=4
     * RX antenna 3: peak for N_eff_TX=1 | peak for N_eff_TX=2 | peak for N_eff_TX=4
     */

    // find the most likely value of N_eff_TX
    uint32_t N_eff_TX_idx_likely = 0;
    float metric_sum_max = 0.0f;
    for (uint32_t N_eff_TX_idx = 0; N_eff_TX_idx < templates.size(); ++N_eff_TX_idx) {
        float metric_sum = 0.0f;

        for (std::size_t i = 0; i < nof_coarse_peak_processable; ++i) {
            const uint32_t ant_idx = coarse_peak_ant_idx_vec[i];
            metric_sum += peak_vec[ant_idx].metric[N_eff_TX_idx];
        }

        // better fitting N_eff_TX?
        if (metric_sum_max < metric_sum) {
            metric_sum_max = metric_sum;
            N_eff_TX_idx_likely = N_eff_TX_idx;
        }
    }

    // convert index to actual value
    sync_report.N_eff_TX = section3::phyres::N_TS_idx_2_N_TS_vec[N_eff_TX_idx_likely];

    // with N_eff_TX determined, we can now find the most likely peak index
    float metric_weighted_index = 0.0f;
    for (std::size_t i = 0; i < nof_coarse_peak_processable; ++i) {
        const uint32_t ant_idx = coarse_peak_ant_idx_vec[i];
        metric_weighted_index += peak_vec[ant_idx].metric[N_eff_TX_idx_likely] *
                                 static_cast<float>(peak_vec[ant_idx].index[N_eff_TX_idx_likely]);
    }

    // save index
    sync_report.fine_peak_time_local =
        static_cast<uint32_t>(std::round(metric_weighted_index / metric_sum_max));
}

}  // namespace dectnrp::phy
