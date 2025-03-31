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

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::phy {

// clang-format off
/* Rational resampler that first interpolates by L and then decimates by M. Input N_TX_max_ is the
 * maximum number of antennas and also the maximum size of output. Input f_pass_norm is required to
 * build shortest low pass filter possible. It must be lower than 0.5 Hz. Internally, the image
 * rejection filter after upsampling is a Kaiser low pass filter. The delay of the Kaiser low pass
 * filter is compensated.
 *
 *      M = 9 and L = 10
 *      h = history
 *      s = skippable samples (always smaller than history, removes filter delay)
 *      z = zeros for flushing
 *      f = filter (length is always #h = #f -1)
 *
 *            |1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8|         N_input_samples
 *        |1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8 M|1 2 3 4 5 6 7 8 M|1|         N_input_samples_effective
 *      |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|0|0|0|
 *       h h h                                                                                         z z z
 *       s
 *        |_|_|_|_|                                                                           |_|_|_|_|
 *         f f f f                                                                             f f f f
 *                                                                                                  |_|_|_|_|   resample_final_samples()
 *                                                                                                   f f f f
 */
// clang-format on
class resampler_t {
    public:
        explicit resampler_t(const uint32_t N_TX_max_,
                             const uint32_t L_,
                             const uint32_t M_,
                             const float f_pass_norm_,
                             const float f_stop_norm_,
                             const float passband_ripple_dB_,
                             const float stopband_attenuation_dB_);
        ~resampler_t();

        resampler_t() = delete;
        resampler_t(const resampler_t&) = delete;
        resampler_t& operator=(const resampler_t&) = delete;
        resampler_t(resampler_t&&) = delete;
        resampler_t& operator=(resampler_t&&) = delete;

        /// we must feed an amount of samples not smaller than the history
        uint32_t get_minimum_nof_input_samples() const { return history_length; };

        /// exact number of output samples for any number of input samples (assuming reset() was
        /// called before feeding new samples)
        uint32_t get_N_samples_after_resampling(const uint32_t N_input_samples) const;

        /// returns the number of newly generated output samples
        uint32_t resample(const std::vector<const cf_t*>& input,
                          std::vector<cf_t*>& output,
                          const uint32_t N_new_input_samples);

        /// returns the number of newly generated output samples, should be called at the end to
        /// flush internal history
        uint32_t resample_final_samples(std::vector<cf_t*>& output);

        /// put into a state so we can start new resampling process
        void reset();

        /// sample_rate * L can be larger than UINT32_MAX, thus, internally a 64 bit multiplication
        /// is performed
        static uint32_t get_samp_rate_converted_with_temporary_overflow(const uint32_t samp_rate,
                                                                        const uint32_t L,
                                                                        const uint32_t M);

        const uint32_t N_TX_max;
        const uint32_t L;
        const uint32_t M;
        const float f_pass_norm;
        const float f_stop_norm;
        const float passband_ripple_dB;
        const float stopband_attenuation_dB;

    private:
        /// dispatcher called in resample()
        uint32_t (resampler_t::*dispatcher_L1M1_LXMX)(const std::vector<const cf_t*>& input,
                                                      std::vector<cf_t*>& output,
                                                      const uint32_t N_new_input_samples) = nullptr;
        uint32_t resample_L1M1(const std::vector<const cf_t*>& input,
                               std::vector<cf_t*>& output,
                               const uint32_t N_new_input_samples);
        uint32_t resample_LXMX(const std::vector<const cf_t*>& input,
                               std::vector<cf_t*>& output,
                               const uint32_t N_new_input_samples);

        uint32_t (resampler_t::*dispatcher_LXMX)(const cf_t* input,
                                                 cf_t* output,
                                                 const uint32_t N_new_input_samples) = nullptr;
        uint32_t resample_LXMX_generic_impl(const cf_t* input,
                                            cf_t* output,
                                            const uint32_t N_new_input_samples);

        // naming convention: L M filter_length filter_length

        uint32_t resample_L10M9_223_23_impl(const cf_t* input,
                                            cf_t* output,
                                            const uint32_t N_new_input_samples);
        uint32_t resample_L9M10_223_25_impl(const cf_t* input,
                                            cf_t* output,
                                            const uint32_t N_new_input_samples);

        uint32_t resample_L10M9_45_5_impl(const cf_t* input,
                                          cf_t* output,
                                          const uint32_t N_new_input_samples);
        uint32_t resample_L9M10_45_5_impl(const cf_t* input,
                                          cf_t* output,
                                          const uint32_t N_new_input_samples);

        /// polyphase subfilters
        uint32_t subfilter_length;
        std::vector<float*> subfilters;

        /// we might have to skip input samples to compensate for low pass filter delay
        uint32_t N_skip_input_samples_front;
        uint32_t N_skip_input_samples_current;

        /// order in which subfilters are used
        common::vec2d<uint32_t> subfilter_indices;

        /// we process and keep history internally
        uint32_t history_length;
        std::vector<cf_t*> history_vec;

        /// individual calls of resample_new_samples() and resample_final_samples() are not
        /// independent
        uint32_t input_sample_cnt;
};

}  // namespace dectnrp::phy
