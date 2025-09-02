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

#include <cstdint>
#include <memory>
#include <vector>

#include "dectnrp/common/complex.hpp"
#include "dectnrp/phy/rx/sync/coarse_peak_f_domain.hpp"
#include "dectnrp/phy/rx/sync/correlator.hpp"
#include "dectnrp/phy/rx/sync/movsum.hpp"
#include "dectnrp/phy/rx/sync/movsum_uw.hpp"

// #define PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT

namespace dectnrp::phy {

class autocorrelator_peak_t final : public correlator_t {
    public:
        explicit autocorrelator_peak_t(const std::vector<cf_t*> localbuffer_,
                                       const uint32_t u_max_,
                                       const uint32_t b_max_,
                                       const uint32_t nof_antennas_limited_,
                                       const uint32_t bos_fac_,
                                       const uint32_t stf_bos_length_samples_,
                                       const uint32_t stf_bos_pattern_length_samples_,
                                       const uint32_t search_length_samples_);
        ~autocorrelator_peak_t();

        autocorrelator_peak_t() = delete;
        autocorrelator_peak_t(const autocorrelator_peak_t&) = delete;
        autocorrelator_peak_t& operator=(const autocorrelator_peak_t&) = delete;
        autocorrelator_peak_t(autocorrelator_peak_t&&) = delete;
        autocorrelator_peak_t& operator=(autocorrelator_peak_t&&) = delete;

        /// put into a state so that we can conduct a coarse peak search
        void set_initial_state(const uint32_t detection_time_with_jump_back_local_);

        uint32_t get_nof_samples_required() const override final;

        bool search_by_correlation(const uint32_t localbuffer_cnt_w,
                                   sync_report_t& sync_report) override final;

    private:
        const uint32_t nof_antennas_limited;
        const uint32_t stf_bos_length_samples;
        const uint32_t stf_bos_pattern_length_samples;
        const uint32_t stf_nof_pattern;

        const uint32_t search_length_samples;

        /**
         * \brief In in earlier version of the TS, the STF was used without a cover sequence and the
         * coarse metric has a long concave shape. As a consequence, the coarse metric was always
         * detected BEFORE the coarse peak and the value of detection2peak_samples was always
         * positive and thus an uint32_t.
         *
         * However, with the new cover sequence, the coarse metric has become very narrow. It is
         * possible that the coarse metric is detected on a falling edge, i.e. BEHIND the coarse
         * peak. For that reason, the variable detection2peak_samples may also be negative. Its type
         * is changed to the next larger signed integer int64_t.
         *
         */
        const int64_t detection2peak_samples_64;

        const float prefactor;

        uint32_t localbuffer_cnt_r_max;

        cf_t* multiplication_stage_correlation;
        cf_t* multiplication_stage_power;

        /// one accumulator per antenna
        std::vector<movsum_uw_t> movsums_correlation;
        std::vector<movsum_t<float>> movsums_power;

        /// smoothing of the metric relevant at low SNR
        const uint32_t metric_smoother_bos_offset_to_center_samples;
        std::vector<movsum_t<float>> metric_smoother;

        [[maybe_unused]] uint32_t resum_cnt;
        void resum_for_numerical_stability();

        /// fill moving sums at detection point
        void set_initial_movsums(const uint32_t start_time_local);

        /// check if there actually was a peak, if so weight height and time
        bool post_processing_validity(sync_report_t& sync_report);

        /// calculations at the coarse peak
        void post_processing_at_coarse_peak(sync_report_t& sync_report);

        /// keeps track of peak search
        class peak_t {
            public:
                peak_t() { reset(); }

                void reset() {
                    metric = 0.0f;
                    index = 0;
                }

                void update_if_metric_is_larger(const float metric_, const uint32_t index_) {
                    if (metric_ >= metric) {
                        metric = metric_;
                        index = index_;
                    }
                };

                float metric;
                uint32_t index;
        };

        /// one peak search per RX antenna
        std::vector<peak_t> peak_vec;

        /// once we have found the coarse peak, we use it to estimate beta and the integer CFO
        std::unique_ptr<coarse_peak_f_domain_t> coarse_peak_f_domain;

#ifdef PHY_RX_AUTOCORRELATOR_PEAK_JSON_EXPORT
        common::vec2d<float> waveform_power;
        common::vec2d<float> waveform_rms;
        common::vec2d<float> waveform_metric;
        common::vec2d<float> waveform_metric_smooth;
        std::vector<uint32_t> waveform_metric_max_idx;

        /// save meta, binary and IQ data of frame on drive
        void write_all_data_to_json(const sync_report_t& sync_report) const;
#endif
};

}  // namespace dectnrp::phy
