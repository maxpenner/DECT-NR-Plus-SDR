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

#pragma once

#include <cstdint>
#include <vector>

#include "dectnrp/phy/rx/sync/correlator.hpp"
#include "dectnrp/phy/rx/sync/movsum.hpp"
#include "dectnrp/phy/rx/sync/movsum_uw.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"

namespace dectnrp::phy {

class autocorrelator_detection_t final : public correlator_t {
    public:
        explicit autocorrelator_detection_t(const std::vector<cf_t*> localbuffer_,
                                            const uint32_t nof_antennas_limited_,
                                            const uint32_t stf_bos_length_samples_,
                                            const uint32_t stf_bos_pattern_length_samples_,
                                            const uint32_t search_length_samples_,
                                            const uint32_t dect_samp_rate_max);
        ~autocorrelator_detection_t() = default;

        autocorrelator_detection_t() = delete;
        autocorrelator_detection_t(const autocorrelator_detection_t&) = delete;
        autocorrelator_detection_t& operator=(const autocorrelator_detection_t&) = delete;
        autocorrelator_detection_t(autocorrelator_detection_t&&) = delete;
        autocorrelator_detection_t& operator=(autocorrelator_detection_t&&) = delete;

        /// put into a state so that we can restart detection for a new chunk
        void reset();

        /// 7 or 9 patterns in an STF, therefore 7 or 9 power values, but only 6 or 8 correlations
        void set_power_of_first_stf_pattern(const uint32_t localbuffer_cnt_w);

        /// avoid re-detecting the same rising edge/peak of the coarse metric
        void skip_after_peak(const uint32_t sync_coarse_or_fine_peak_time_local);

        uint32_t get_nof_samples_required() const override final;

        bool search_by_correlation(const uint32_t localbuffer_cnt_w,
                                   sync_report_t& sync_report) override final;

        const uint32_t nof_antennas_limited;
        const uint32_t stf_bos_pattern_length_samples;
        const uint32_t stf_nof_pattern;

        const uint32_t search_step_samples;
        const uint32_t search_length_samples;

    private:
        const uint32_t search_jump_back_samples;
        const uint32_t search_start_samples;

        // ##################################################
        // accumulation of power and correlation

        /// detection accumulators, one accumulator per antenna
        std::vector<movsum_uw_t> movsums_correlation;
        std::vector<movsum_t<float>> movsums_power;

#ifdef RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RESUM_PERIODICITY_IN_STEPS
        uint32_t resum_cnt;
        void resum_for_numerical_stability();
#endif

        // ##################################################
        // coarse metric threshold checking

        uint32_t ignore_before_index;

        const float power_normalizer;
        const float rms_minimum;

        const float correlation_prefactor;

        class streak_t {
            public:
                streak_t(const float value_start_,
                         const float value_multiplier_,
                         const uint32_t cnt_goal_);

                bool check(const float value_);
                void reset();

            private:
                float value_start;
                float value_multiplier;
                float value;

                uint32_t cnt;
                uint32_t cnt_goal;
        };

        std::vector<streak_t> metric_streak_vec;

        void reset_metric_streak_vec();

        // ##################################################
        // post peak behaviour

        const uint32_t skip_after_peak_samples;
};

}  // namespace dectnrp::phy
