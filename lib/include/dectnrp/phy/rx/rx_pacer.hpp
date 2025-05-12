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
#include "dectnrp/phy/resample/resampler.hpp"
#include "dectnrp/phy/rx/localbuffer.hpp"
#include "dectnrp/radio/buffer_rx.hpp"

#ifdef ENABLE_ASSERT
#include "dectnrp/common/adt/miscellaneous.hpp"
#endif

namespace dectnrp::phy {

class rx_pacer_t {
    public:
        virtual ~rx_pacer_t();

        rx_pacer_t() = delete;
        rx_pacer_t(const rx_pacer_t&) = delete;
        rx_pacer_t& operator=(const rx_pacer_t&) = delete;
        rx_pacer_t(rx_pacer_t&&) = delete;
        rx_pacer_t& operator=(rx_pacer_t&&) = delete;

        /// pacer holds two kinds of localbuffers
        enum class localbuffer_choice_t {
            LOCALBUFFER_FILTER = 0,   // filter at hardware sample rate
            LOCALBUFFER_RESAMPLE = 1  // resample to DECTNRP sample rate
        };

        const uint32_t nof_antennas_limited;
        const uint32_t ant_streams_length_samples;
        const uint32_t ant_streams_unit_length_samples;

    private:
        /// read-only access to hardware samples
        const radio::buffer_rx_t& buffer_rx;
        const std::vector<const cf_t*> ant_streams;

        /// container for samples from right and left edge of ant_streams for consecutive processing
        std::vector<cf_t*> ant_streams_edge;

        /// container to hold pointers with offsets in ant_streams or any localbuffer
        std::vector<const cf_t*> ant_streams_offset;
        std::vector<cf_t*> localbuffer_offset;

        // ##################################################
        // LOCALBUFFER_FILTER

        std::unique_ptr<localbuffer_t> lb_filter;
        void filter_single_unit();

        // ##################################################
        // LOCALBUFFER_RESAMPLE

        const std::unique_ptr<resampler_t> resampler;
        std::unique_ptr<localbuffer_t> lb_resampler;
        void resample_single_unit();

    protected:
        /**
         * \brief Class translates between buffer_rx_t and local buffers, and between different
         * samples rates. Note that this class has full access to all samples in buffer_rx_t,
         * however, the total number of antennas used for translation can be limited by
         * nof_antennas_limited_. The argument nof_antennas_limited_ is always smaller than the
         * number of physical antennas.
         *
         * \param nof_antennas_limited_ number of antennas used for STF search
         * \param buffer_rx_ ring buffer filled by radio layer
         * \param ant_streams_unit_length_samples_ number of samples resampler processes per call
         * \param resampler_
         */
        explicit rx_pacer_t(const uint32_t nof_antennas_limited_,
                            const radio::buffer_rx_t& buffer_rx_,
                            const uint32_t ant_streams_unit_length_samples_,
                            resampler_t* resampler_);

        /**
         * \brief Based on the maximum number of samples that a deriving class requires in the local
         * buffer, this function initializes the internal local buffer type.
         *
         * \param lbc which buffer to use
         * \param localbuffer_length_samples_max minimum number of samples required in the buffer
         * \return
         */
        std::vector<cf_t*> get_initialized_localbuffer(
            const localbuffer_choice_t lbc, const uint32_t localbuffer_length_samples_max);

        /// bring into default state
        void reset_localbuffer(const localbuffer_choice_t lbc, const int64_t ant_streams_time_64_);

#ifdef ENABLE_ASSERT
        int64_t search_time_start_64{common::adt::UNDEFINED_EARLY_64};
        void check_time_lag(const int64_t time_to_check_for_lag_64) const;
#endif

        /// blocks until global_time_64 is reached, wrapper of buffer_rx functions, nto = no timeout
        void wait_until_nto(const int64_t global_time_64) const;

        // ##################################################
        // LOCALBUFFER_FILTER

        // nto = no timeout
        uint32_t filter_until_nto(const uint32_t cnt_w_min);

        // ##################################################
        // LOCALBUFFER_RESAMPLE

        /// force next translation to write to beginning of local buffer
        void rewind_localbuffer_resample_cnt_w();

        /// polls buffer_rx and resamples until at least the specific amount of samples was
        /// generated, returns actual amount, nto = no timeout
        uint32_t resample_until_nto(const uint32_t cnt_w_min);

        /// length conversion, depends on resampler, rounding to nearest integer
        uint32_t convert_length_global_to_resampled(const uint32_t global_length) const;
        uint32_t convert_length_resampled_to_global(const uint32_t resampled_length) const;

        /// time conversion, depends on resampler
        uint32_t convert_time_global_to_resampled(const int64_t global_time_64,
                                                  const int64_t global_time_offset_64 = 0) const;
        int64_t convert_time_resampled_to_global(const uint32_t resampled_time,
                                                 const int64_t global_time_offset_64 = 0) const;
};

}  // namespace dectnrp::phy
