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
#include <memory>
#include <vector>

#include "dectnrp/common/complex.hpp"
#include "dectnrp/phy/mix/mixer.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/phy/rx/sync/correlator.hpp"
#include "dectnrp/phy/rx/sync/stf_template.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"

namespace dectnrp::phy {

class crosscorrelator_t final : public correlator_t {
    public:
        explicit crosscorrelator_t(const std::vector<cf_t*> localbuffer_,
                                   const uint32_t u_max_,
                                   const uint32_t b_max_,
                                   const uint32_t os_min_,
                                   const uint32_t nof_antennas_,
                                   const uint32_t nof_antennas_limited_,
                                   const resampler_param_t resampler_param_);
        ~crosscorrelator_t();

        crosscorrelator_t() = delete;
        crosscorrelator_t(const crosscorrelator_t&) = delete;
        crosscorrelator_t& operator=(const crosscorrelator_t&) = delete;
        crosscorrelator_t(crosscorrelator_t&&) = delete;
        crosscorrelator_t& operator=(crosscorrelator_t&&) = delete;

        /// put into a state so that we can start a new fine search
        void set_initial_state();

        uint32_t get_nof_samples_required() const override final;

        bool search_by_correlation(const uint32_t localbuffer_cnt_w,
                                   sync_report_t& sync_report) override final;

        const uint32_t nof_antennas;
        const uint32_t nof_antennas_limited;
        const resampler_param_t resampler_param;

        /**
         * \brief Scheme for search range:
         *
         * 012345678901234567------------
         * lllllxrrrrrrr-----------------
         *      012345678901234567-------
         *             012345678901234567
         *
         * stf_bos_length_samples = 18
         * search_length_l = 5
         * search_length_r = 7
         * search_length = 5+7+1=13
         *
         * mixer_stage_len = 18+5+7=30
         */
        const uint32_t search_length_l;
        const uint32_t search_length_r;
        const uint32_t search_length;

    private:
        const std::unique_ptr<stf_template_t> stf_template;

        /// STF must be derotated before searching for fine peaks
        const uint32_t mixer_stage_len;
        std::vector<cf_t*> mixer_stage;
        mixer_t mixer;

        /// results container for cross correlation
        cf_t* xcorr_stage;

        void run_fine_search(sync_report_t& sync_report);

        /// keeps track of peak search
        class peak_t {
            public:
                peak_t() = default;
                explicit peak_t(const uint32_t N_eff_TX_idx_max_) {
                    metric.resize(N_eff_TX_idx_max_);
                    index.resize(N_eff_TX_idx_max_);
                    reset();
                }

                void reset() {
                    std::fill(metric.begin(), metric.end(), 0.0f);
                    std::fill(index.begin(), index.end(), 0);
                }

                void set_metric_index(const float metric_,
                                      const uint32_t index_,
                                      const uint32_t N_eff_TX_idx) {
                    metric[N_eff_TX_idx] = metric_;
                    index[N_eff_TX_idx] = index_;
                };

                /// one metric and one index for each possible value of N_eff_TX
                std::vector<float> metric;
                std::vector<uint32_t> index;
        };

        /// one peak search per RX antenna
        std::vector<peak_t> peak_vec;
};

}  // namespace dectnrp::phy
