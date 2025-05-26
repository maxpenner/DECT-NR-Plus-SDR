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

#include "dectnrp/phy/rx/sync/autocorrelator_detection.hpp"

#include <volk/volk.h>

#include <algorithm>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"
#include "dectnrp/sections_part3/stf.hpp"

namespace dectnrp::phy {

autocorrelator_detection_t::autocorrelator_detection_t(
    const std::vector<cf_t*> localbuffer_,
    const uint32_t nof_antennas_limited_,
    const uint32_t stf_bos_length_samples_,
    const uint32_t stf_bos_pattern_length_samples_,
    const uint32_t search_length_samples_,
    const uint32_t dect_samp_rate_max)
    : correlator_t(localbuffer_),

      nof_antennas_limited(nof_antennas_limited_),
      stf_bos_pattern_length_samples(stf_bos_pattern_length_samples_),
      stf_nof_pattern(stf_bos_length_samples_ / stf_bos_pattern_length_samples),

      search_step_samples(stf_bos_pattern_length_samples /
                          RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER),
      search_length_samples(search_length_samples_),

      search_jump_back_samples(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_JUMP_BACK_IN_PATTERNS *
                               stf_bos_pattern_length_samples),
      search_start_samples(stf_bos_length_samples_ + search_jump_back_samples),

      power_normalizer(static_cast<float>(stf_bos_length_samples_)),

      /* Scale minimum RMS: A smaller bandwidth implies a smaller minimum RMS required. Note that
       * half the bandwidth has half the noise power, i.e. sqrt(0.5) is the RMS.
       */
      rms_minimum(
          RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MIN_SP *
          std::sqrt(
              static_cast<double>(dect_samp_rate_max) /
              RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MIN_REFERENCE_SAMPLE_RATE_DP)),

      correlation_prefactor(static_cast<float>(stf_nof_pattern) /
                            static_cast<float>(stf_nof_pattern - 1)),

      skip_after_peak_samples(
          static_cast<uint32_t>(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_SKIP_AFTER_PEAK_IN_STFS_DP *
                                static_cast<double>(stf_bos_length_samples_))) {
    dectnrp_assert(
        constants::N_samples_stf_pattern % RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER == 0,
        "base length of pattern must be divisible by step divider");

    movsums_correlation.resize(nof_antennas_limited);
    movsums_power.resize(nof_antennas_limited);

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        movsums_correlation[ant_idx] =
            movsum_uw_t(sp3::stf_t::get_cover_sequence_pairwise_product(
                            sp3::stf_t::get_equivalent_u(stf_nof_pattern)),
                        RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER);

        movsums_power[ant_idx] =
            movsum_t<float>(stf_nof_pattern * RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER);

        metric_streak_vec.push_back(
            streak_t(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_THRESHOLD_MIN_SP,
                     RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_STREAK_RELATIVE_GAIN_SP,
                     RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_STREAK));
    }
}

void autocorrelator_detection_t::reset() {
    localbuffer_cnt_r = 0;

    for (auto& elem : movsums_correlation) {
        elem.reset();
    }

    for (auto& elem : movsums_power) {
        elem.reset();
    }

#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RESUM_PERIODICITY_IN_STEPS
    resum_cnt = 0;
#endif

    ignore_before_index = search_start_samples;

    reset_metric_streak_vec();
}

void autocorrelator_detection_t::set_power_of_first_stf_pattern(const uint32_t localbuffer_cnt_w) {
    dectnrp_assert(localbuffer_cnt_r == 0, "localbuffer_cnt_r not zero.");
    dectnrp_assert(stf_bos_pattern_length_samples <= localbuffer_cnt_w, "not enough samples");

    lv_32fc_t result;

    for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
        for (uint32_t i = 0; i < RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER; ++i) {
            // readability pointer
            const cf_t* A = &localbuffer[ant_idx][i * search_step_samples];

            volk_32fc_x2_conjugate_dot_prod_32fc(
                &result, (const lv_32fc_t*)A, (const lv_32fc_t*)A, search_step_samples);

            movsums_power[ant_idx].pop_push(result.real());
        }
    }

    localbuffer_cnt_r = stf_bos_pattern_length_samples;
}

void autocorrelator_detection_t::skip_after_peak(
    const uint32_t sync_coarse_or_fine_peak_time_local) {
    dectnrp_assert(
        ignore_before_index < sync_coarse_or_fine_peak_time_local + skip_after_peak_samples,
        "skipping must advance ignore_before_index");

    ignore_before_index = sync_coarse_or_fine_peak_time_local + skip_after_peak_samples;

    reset_metric_streak_vec();
}

uint32_t autocorrelator_detection_t::get_nof_samples_required() const {
    return localbuffer_cnt_r + search_step_samples;
}

bool autocorrelator_detection_t::search_by_correlation(const uint32_t localbuffer_cnt_w,
                                                       sync_report_t& sync_report) {
    lv_32fc_t result;

    // keep consuming samples as long as we enough samples for another full step
    while (localbuffer_cnt_r + search_step_samples <= localbuffer_cnt_w) {
        // calculate power and correlation for each antenna, and put into moving sums
        for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
            // readability pointers
            const cf_t* A =
                &localbuffer[ant_idx][localbuffer_cnt_r - stf_bos_pattern_length_samples];
            const cf_t* B = &localbuffer[ant_idx][localbuffer_cnt_r];

            volk_32fc_x2_conjugate_dot_prod_32fc(
                &result, (const lv_32fc_t*)A, (const lv_32fc_t*)B, search_step_samples);

            movsums_correlation[ant_idx].pop_push(result);

            volk_32fc_x2_conjugate_dot_prod_32fc(
                &result, (const lv_32fc_t*)B, (const lv_32fc_t*)B, search_step_samples);

            movsums_power[ant_idx].pop_push(result.real());
        }

#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RESUM_PERIODICITY_IN_STEPS
        // resum accumulator to avoid numerical instability
        if (resum_cnt++ == RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RESUM_PERIODICITY_IN_STEPS) {
            resum_for_numerical_stability();
            resum_cnt = 0;
        }
#endif

        localbuffer_cnt_r += search_step_samples;

        /* At this point, we went ahead another step and we have to check whether we might have
         * detected another packet at any of the antennas.
         */

        // do we even have to check the metric?
        if (ignore_before_index <= localbuffer_cnt_r) {
            for (uint32_t ant_idx = 0; ant_idx < nof_antennas_limited; ++ant_idx) {
                /* The first detection condition is an RMS that stays within reasonable limits, i.e.
                 * it's not too large and not too small. We consider the RMS across the full
                 * correlation window.
                 */

                // power across full correlation window
                const float power = movsums_power[ant_idx].get_sum();

                // average/expected rms of a single sample
                const float rms = std::sqrt(power / power_normalizer);

                // limited RMS
                if (rms < rms_minimum ||
                    RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_THRESHOLD_MAX_SP < rms) {
                    continue;
                }

                /* The second detection condition is an increasing RMS within the correlation
                 * window. A specific number of front elements must have a higher RMS than a
                 * specific number of back elements. This is very important if packets are received
                 * with different power levels, especially a strong packet followed by a weak
                 * packet.
                 */

                // RMS of back elements
                const float rms_back = std::sqrt(movsums_power[ant_idx].get_sum_back(
                    RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_BACK_STEPS));

                // power of front elements
                const float rms_front = std::sqrt(movsums_power[ant_idx].get_sum_front(
                    RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_FRONT_STEPS));

                // compare
                if (rms_back * RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_FRONT_TO_BACK_RATIO >=
                    rms_front) {
                    continue;
                }

                /* At this point, we know that the RMS lies within reasonable limits and it is
                 * increasing.
                 *
                 * Our third detection condition is a coarse metric within reasonable limit. So
                 * next, we extract the correlation and normalize with the power. The equation is
                 * taken from "A Robust Timing and Frequency Synchronization for OFDM Systems",
                 * Hlaing Minn, Equation (14).
                 */
                const std::complex<float> correlation(movsums_correlation[ant_idx].get_sum());

                // normalize to range 0 to 1.0
                const float metric =
                    powf(correlation_prefactor * std::abs(correlation) / power, 2.0f);

                // limited metric
                if (metric < RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_THRESHOLD_MIN_SP ||
                    RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_THRESHOLD_MAX_SP < metric) {
                    metric_streak_vec[ant_idx].reset();
                    continue;
                }

                /* The fourth and final detection condition is a streak of increasing correlation
                 * windows. With this, we can make sure that we are detecting a rising edge of the
                 * coarse metric. However, if a non-trivial cover sequence is used, the metric may
                 * become so slim that the expected streak length must be made shorter.
                 */

                // we need a streak of increasing metric values
                if (!metric_streak_vec[ant_idx].check(metric)) {
                    continue;
                }

                // overwrite values in sync_report
                sync_report.detection_ant_idx = ant_idx;
                sync_report.detection_rms = rms;
                sync_report.detection_metric = metric;
                sync_report.detection_time_local = localbuffer_cnt_r;
                sync_report.detection_time_with_jump_back_local =
                    sync_report.detection_time_local - search_jump_back_samples;

                // leave prematurely as immediate action is required
                return true;
            }
        }

        // leave prematurely as full search length has been covered
        if (search_length_samples <= localbuffer_cnt_r) {
            return false;
        }
    }

    return false;
}

#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RESUM_PERIODICITY_IN_STEPS
void autocorrelator_detection_t::resum_for_numerical_stability() {
    for (auto& elem : movsums_correlation) {
        elem.resum();
    }

    for (auto& elem : movsums_power) {
        elem.resum();
    }
}
#endif

void autocorrelator_detection_t::reset_metric_streak_vec() {
    for (auto& elem : metric_streak_vec) {
        elem.reset();
    }
}

autocorrelator_detection_t::streak_t::streak_t(const float value_start_,
                                               const float value_multiplier_,
                                               const uint32_t cnt_goal_)
    : value_start(value_start_),
      value_multiplier(value_multiplier_),
      value(value_start),
      cnt(0),
      cnt_goal(cnt_goal_) {
    dectnrp_assert(cnt_goal >= 1, "streak must have a minimum length of 1");
};

bool autocorrelator_detection_t::streak_t::check(const float value_) {
    if (value < value_) {
        ++cnt;
        value = std::max(value_ * value_multiplier, value_start);
        return cnt >= cnt_goal;
    }

    reset();
    return false;
}

void autocorrelator_detection_t::streak_t::reset() {
    value = value_start;
    cnt = 0;
}

}  // namespace dectnrp::phy
