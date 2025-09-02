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

#include "dectnrp/phy/resample/resampler.hpp"

#include <volk/volk.h>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/phy/filter/kaiser.hpp"

// #define RESAMPLER_LOG_TO_SIMPLIFY_HANDCRAFTING_UNROLLED_LOOPS

#ifdef RESAMPLER_LOG_TO_SIMPLIFY_HANDCRAFTING_UNROLLED_LOOPS
#include "dectnrp/common/prog/log.hpp"
#endif

#define MANUALLY_UNROLLED_FRONT                                            \
    uint32_t cnt_in = std::min(N_new_input_samples, M - input_sample_cnt); \
    uint32_t cnt_out = resample_LXMX_generic_impl(input, output, cnt_in);  \
    const uint32_t full_M = (N_new_input_samples - cnt_in) / M;            \
    cf_t* A = &output[cnt_out];                                            \
    const cf_t* B = &input[cnt_in];

#define MANUALLY_UNROLLED_BACK                                               \
    if (cnt_in < N_new_input_samples) {                                      \
        cnt_out += resample_LXMX_generic_impl(                               \
            &input[cnt_in], &output[cnt_out], N_new_input_samples - cnt_in); \
    }                                                                        \
    return cnt_out;

namespace dectnrp::phy {

resampler_t::resampler_t(const uint32_t N_TX_max_,
                         const uint32_t L_,
                         const uint32_t M_,
                         const float f_pass_norm_,
                         const float f_stop_norm_,
                         const float passband_ripple_dB_,
                         const float stopband_attenuation_dB_)
    : N_TX_max(N_TX_max_),
      L(L_),
      M(M_),
      f_pass_norm(f_pass_norm_),
      f_stop_norm(f_stop_norm_),
      passband_ripple_dB(passband_ripple_dB_),
      stopband_attenuation_dB(stopband_attenuation_dB_) {
    // effectively no resampling, but we still acccept these parameters for convenience
    if (L == 1 && M == 1) {
        dispatcher_L1M1_LXMX = &resampler_t::resample_L1M1;

        // these variables must be set to get correct results in get_N_samples_after_resampling()
        history_length = 0;
        N_skip_input_samples_front = 0;

    } else {
        dispatcher_L1M1_LXMX = &resampler_t::resample_LXMX;

        dectnrp_assert(0.0f < f_pass_norm && f_pass_norm < 0.5f,
                       "f_pass_norm must be normalized and between 0 and 0.5");
        dectnrp_assert(f_pass_norm < f_stop_norm, "f_pass_norm must be smaller f_stop_norm");

        // image filter and anti aliasing filter are merged into one, choose the narrower
        const float L_or_M_larger = std::max(static_cast<float>(L), static_cast<float>(M));

        // low pass filter after upsampling
        const float f_pass = f_pass_norm / L_or_M_larger;
        const float f_stop = f_stop_norm / L_or_M_larger;

        // determine real-valued coefficients of Kaiser window
        std::vector<float> low_pass_filter_coefficients =
            filter::kaiser(f_pass, f_stop, passband_ripple_dB, stopband_attenuation_dB, 1.0f, true);

        const uint32_t filter_length = low_pass_filter_coefficients.size();

        dectnrp_assert(filter_length % 2 != 0, "filter length must be odd");

        const uint32_t filter_delay = (filter_length - 1) / 2;

        /* The input signal to the resampler has an average power of 1.
         * When upscaling, we insert L-1 zeros between samples, so we have an average power of 1/L.
         * The upsampled signal is then filtered with the low pass filter, which additionally
         * removes all spectrum copies above the normalized frequency 1/L. So, after low pass
         * fitering, we end up with an average power of 1/L^2. To compensate, we have to increase
         * the amplitude of the coefficients by L.
         *
         * When called by TX, it is L > M. TX samples up by L (usually 10 or 40) and we have to
         * adjust the power accordingly. When called by RX, it is L < M. RX samples up by L (usually
         * 9 or 27) and we have to adjust the power accordingly.
         */
        for (auto& elem : low_pass_filter_coefficients) {
            elem *= L;
        }

        // each of the L subfilters must have the same length, therefore we might have to append
        // zeros
        const uint32_t n_append_zeros =
            common::adt::ceil_divide_integer(filter_length, L) * L - filter_length;

        for (uint32_t i = 0; i < n_append_zeros; ++i) {
            low_pass_filter_coefficients.push_back(0.0f);
        }

        subfilter_length = (filter_length + n_append_zeros) / L;

        // initialize L polyphase subfilters
        for (uint32_t l = 0; l < L; ++l) {
            float* subfilter = srsran_vec_f_malloc(subfilter_length);

            // each filter is used in a convolution with the input samples, so filter is mirrored
            for (uint32_t k = 0; k < subfilter_length; ++k) {
                subfilter[subfilter_length - 1 - k] = low_pass_filter_coefficients[l + k * L];
            }

            subfilters.push_back(subfilter);
        }

        // clang-format off
        /* Determine order in which the subfilters are called to produce output samples.
         *
         * Example 0 upsampling:
         *
         * When L=10 and M=9, for each 9 input samples we produce 10 output samples.
         * x stands for the filter delay of low pass filter, which is skipped.
         *
         * The correct order then is:
         *
         *                              input sample index
         *                      12  11  10  9   8   7   6   5   4   3   2   1   0
         *
         * subfilter index:
         *      0                                   6                       x   x
         *      1                                       5                   x   x
         *      2                                           4               x   x
         *      3                                               3           x   x
         *      4               ...                                 2       x   x
         *      5                   11                                  1   x   x
         *      6                       10            first output sample-> 0   x
         *      7                           9                                   x
         *      8                               8                               x
         *      9                                   7                           x
         *
         *
         * Known limitation: Does not work if the filter delay ends for an input sample index with
         * two output samples (index 7 in image above). Sanity check below fails in that case.
         * Slightly adjust filter length.
         *
         *
         * Example 1 downsampling:
         *
         * When L=9 and M=10, for each 10 input samples we produce 9 output samples.
         * x stands for the filter delay of low pass filter, which is skipped.
         *
         * The correct order then is:
         *
         *                              input sample index
         *                      12  11  10  9   8   7   6   5   4   3   2   1   0
         *
         * subfilter index:
         *      0                                           3               x   x
         *      1                                       4                   x   x
         *      2                                   5                       x   x
         *      3                               6                           x   x
         *      4               ...         7                               x   x
         *      5                        8                                  x   x
         *      6                    9                first output sample-> 0   x
         *      7                                                       1       x
         *      8                                                   2           x
         */
        // clang-format on

        // in the figure above, the first few input samples are not used to produce any output
        // samples, therefore skippable
        N_skip_input_samples_front = filter_delay / L;

        // this is the subfilter index of the first output sample (in the figure above it's 6)
        uint32_t subfilter_index = filter_delay % L;

        // get the subfilter indices for next output samples (in the figure above 5, 4, 3, 2, 1, 0,
        // 9, 8, 7)
        for (uint32_t input_sample_period = 0; input_sample_period < M; ++input_sample_period) {
            std::vector<uint32_t> input_samples_index_to_subfilter_index;

            while (subfilter_index < L) {
                input_samples_index_to_subfilter_index.push_back(subfilter_index);
                subfilter_index += M;
            }

            subfilter_indices.push_back(input_samples_index_to_subfilter_index);

            // when using downsampling, e.g. L=9 and M=10, it can happen that for an input sample NO
            // output sample is generated
            if (subfilter_index >= 2 * L) {
                subfilter_indices.emplace_back(std::vector<uint32_t>());
                ++input_sample_period;
            }

            subfilter_index %= L;
        }

        // assert number of subfilters used, for M input symbols L output symbols must be generated
        uint32_t N_output_samples_per_M_input_samples = 0;
        for (uint32_t i = 0; i < subfilter_indices.size(); ++i) {
            N_output_samples_per_M_input_samples += subfilter_indices[i].size();
        }
        dectnrp_assert(N_output_samples_per_M_input_samples == L,
                       "incorrect number of output samples per M input samples {} {} {}",
                       L,
                       M,
                       f_pass_norm_);

        history_length = subfilter_length - 1;

        dectnrp_assert(N_skip_input_samples_front <= history_length,
                       "history shorter than number of skippable samples");

        // create history containers for each antenna
        for (uint32_t i = 0; i < N_TX_max; ++i) {
            history_vec.push_back(srsran_vec_cf_malloc(2 * history_length));
        }

#ifdef RESAMPLER_LOG_TO_SIMPLIFY_HANDCRAFTING_UNROLLED_LOOPS
        dectnrp_log_inf("");
        dectnrp_log_inf(
            "L={} M={} f_pass_norm_={:.5f} f_stop_norm={:.5f} passband_ripple_dB={:.5f} stopband_attenuation_dB={:.5f}",
            L,
            M,
            f_pass_norm,
            f_stop_norm,
            passband_ripple_dB,
            stopband_attenuation_dB);
        dectnrp_log_inf("filter_length={} subfilter_length={}", filter_length, subfilter_length);

        // print subfilter indices
        for (size_t i = 0; i < subfilter_indices.size(); ++i) {
            dectnrp_log_inf(
                "sample_index={} Number of Subfilters={}", i, subfilter_indices[i].size());
            for (size_t j = 0; j < subfilter_indices[i].size(); ++j) {
                dectnrp_log_inf("    subfilter_index={}", subfilter_indices[i][j]);
            }
        }
        dectnrp_log_inf("");
#endif

        // set correct dispatcher value
        if (L == 10 && M == 9 && filter_length == 223) {
            dispatcher_LXMX = &resampler_t::resample_L10M9_223_23_impl;
        } else if (L == 9 && M == 10 && filter_length == 223) {
            dispatcher_LXMX = &resampler_t::resample_L9M10_223_25_impl;
        } else if (L == 10 && M == 9 && filter_length == 45) {
            dispatcher_LXMX = &resampler_t::resample_L10M9_45_5_impl;
        } else if (L == 9 && M == 10 && filter_length == 45) {
            dispatcher_LXMX = &resampler_t::resample_L9M10_45_5_impl;
        } else {
            dispatcher_LXMX = &resampler_t::resample_LXMX_generic_impl;

            /* Fractional resampling with a polyphase filter is very expensive in terms of
             * computational load. To optimize performance, we use manually unrolled loops which are
             * usable only for specific filter configurations.
             *
             * If at any stage in the transceiver the slower generic version of the resampler is
             * used, we write a warning to the log file.
             */
            dectnrp_log_wrn(
                "Generic Resampler for L={} M={} f_pass_norm_={:.5f} f_stop_norm={:.5f} passband_ripple_dB={:.5f} stopband_attenuation_dB={:.5f}",
                L,
                M,
                f_pass_norm,
                f_stop_norm,
                passband_ripple_dB,
                stopband_attenuation_dB);
        }
    }

    reset();
}

resampler_t::~resampler_t() {
    for (auto& elem : subfilters) {
        free(elem);
    }
    subfilters.clear();

    for (auto& elem : history_vec) {
        free(elem);
    }
    history_vec.clear();
}

uint32_t resampler_t::get_N_samples_after_resampling(const uint32_t N_input_samples) const {
    const uint32_t N_input_samples_effective =
        history_length + N_input_samples - N_skip_input_samples_front;

    const uint32_t M_multiples = N_input_samples_effective / M;

    const uint32_t N_residual_samples = N_input_samples_effective - M_multiples * M;

    uint32_t N_resampled_samples = M_multiples * L;

    // each residual input sample can generate zero, one or more samples
    for (uint32_t i = 0; i < N_residual_samples; ++i) {
        N_resampled_samples += subfilter_indices[i].size();
    }

    return N_resampled_samples;
}

uint32_t resampler_t::resample(const std::vector<const cf_t*>& input,
                               std::vector<cf_t*>& output,
                               const uint32_t N_new_input_samples) {
    return (*this.*dispatcher_L1M1_LXMX)(input, output, N_new_input_samples);
}

uint32_t resampler_t::resample_final_samples(std::vector<cf_t*>& output) {
    // configured to do no resampling at all
    if (L == 1 && M == 1) {
        return 0;
    }

    const uint32_t input_sample_cnt_copy = input_sample_cnt;

    uint32_t out_cnt = 0;

    for (uint32_t i = 0; i < output.size(); ++i) {
        input_sample_cnt = input_sample_cnt_copy;

        // zero second half as use it as termination zeros
        srsran_vec_cf_zero(&history_vec[i][history_length], history_length);

        // final samples flushing the history
        out_cnt = resample_LXMX_generic_impl(&history_vec[i][0], output[i], history_length);
    }

    return out_cnt;
}

void resampler_t::reset() {
    // zero first half
    for (auto& elem : history_vec) {
        srsran_vec_cf_zero(elem, history_length);
    }

    N_skip_input_samples_current = N_skip_input_samples_front;
    input_sample_cnt = 0;
}

uint32_t resampler_t::get_samp_rate_converted_with_temporary_overflow(const uint32_t samp_rate,
                                                                      const uint32_t L,
                                                                      const uint32_t M) {
    dectnrp_assert(samp_rate % M == 0, "sample rate before resampling not divided by M");

    const uint64_t samp_rate_after_resampling_64 =
        static_cast<uint64_t>(samp_rate) * static_cast<uint64_t>(L) / static_cast<uint64_t>(M);

    dectnrp_assert(samp_rate_after_resampling_64 <= static_cast<uint64_t>(UINT32_MAX),
                   "overflow for samp_rate_before_resampling");

    return static_cast<uint32_t>(samp_rate_after_resampling_64);
}

uint32_t resampler_t::resample_L1M1(const std::vector<const cf_t*>& input,
                                    std::vector<cf_t*>& output,
                                    const uint32_t N_new_input_samples) {
    for (uint32_t i = 0; i < output.size(); ++i) {
        srsran_vec_cf_copy(output[i], input[i], N_new_input_samples);
    }

    return N_new_input_samples;
}

uint32_t resampler_t::resample_LXMX(const std::vector<const cf_t*>& input,
                                    std::vector<cf_t*>& output,
                                    const uint32_t N_new_input_samples) {
    dectnrp_assert(N_new_input_samples >= history_length,
                   "number of input samples smaller than history");

    // same across all antennas, but not the same across function calls
    const uint32_t input_sample_cnt_copy = input_sample_cnt;
    const uint32_t N_skip_input_samples_current_copy = N_skip_input_samples_current;

    // output samples produced
    uint32_t out_cnt = 0;

    // iterate over each antenna
    for (uint32_t i = 0; i < output.size(); ++i) {
        input_sample_cnt = input_sample_cnt_copy;

        // copy new samples next to history to process these samples
        srsran_vec_cf_copy(&history_vec[i][history_length], input[i], history_length);

        // resample history
        out_cnt = (*this.*dispatcher_LXMX)(&history_vec[i][N_skip_input_samples_current_copy],
                                           output[i],
                                           history_length - N_skip_input_samples_current_copy);

        // resample new samples
        out_cnt += (*this.*dispatcher_LXMX)(
            input[i], &output[i][out_cnt], N_new_input_samples - history_length);

        // overwrite history
        srsran_vec_cf_copy(
            &history_vec[i][0], &input[i][N_new_input_samples - history_length], history_length);
    }

    // set to zero for next calls
    N_skip_input_samples_current = 0;

    return out_cnt;
}

uint32_t resampler_t::resample_LXMX_generic_impl(const cf_t* input,
                                                 cf_t* output,
                                                 const uint32_t N_new_input_samples) {
    uint32_t cnt_in = 0;
    uint32_t cnt_out = 0;

    for (uint32_t i = 0; i < N_new_input_samples; ++i) {
        // some input samples can generate more than one output sample
        for (uint32_t j = 0; j < subfilter_indices[input_sample_cnt].size(); ++j) {
            volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)&output[cnt_out++],
                                          (const lv_32fc_t*)&input[cnt_in],
                                          subfilters[subfilter_indices[input_sample_cnt][j]],
                                          subfilter_length);
        }

        ++cnt_in;

        input_sample_cnt = (input_sample_cnt + 1) % M;
    }

    return cnt_out;
}

uint32_t resampler_t::resample_L10M9_223_23_impl(const cf_t* input,
                                                 cf_t* output,
                                                 const uint32_t N_new_input_samples) {
    MANUALLY_UNROLLED_FRONT;

    // run manually unrolled loop
    for (uint32_t i = 0; i < full_M; ++i) {
        // clang-format off
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[1], 23);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B,   subfilters[0], 23); // two output samples for one input sample
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[9], 23); // two output samples for one input sample
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[8], 23); 
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[7], 23);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[6], 23);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[5], 23);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[4], 23);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[3], 23);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[2], 23);
        // clang-format on

        cnt_in += M;
        cnt_out += L;
    }

    MANUALLY_UNROLLED_BACK;
}

uint32_t resampler_t::resample_L9M10_223_25_impl(const cf_t* input,
                                                 cf_t* output,
                                                 const uint32_t N_new_input_samples) {
    MANUALLY_UNROLLED_FRONT;

    // run manually unrolled loop
    for (uint32_t i = 0; i < full_M; ++i) {
        // clang-format off
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[3], 25);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[4], 25);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[5], 25);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[6], 25);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[7], 25);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[8], 25);
        B++; //volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[7], 25);   // no output sample for one input sample
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[0], 25);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[1], 25);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[2], 25);
        // clang-format on

        cnt_in += M;
        cnt_out += L;
    }

    MANUALLY_UNROLLED_BACK;
}

uint32_t resampler_t::resample_L10M9_45_5_impl(const cf_t* input,
                                               cf_t* output,
                                               const uint32_t N_new_input_samples) {
    MANUALLY_UNROLLED_FRONT;

    // run manually unrolled loop
    for (uint32_t i = 0; i < full_M; ++i) {
        // clang-format off
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[2], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[1], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B,   subfilters[0], 5); // two output samples for one input sample
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[9], 5); // two output samples for one input sample
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[8], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[7], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[6], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[5], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[4], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[3], 5);
        // clang-format on

        cnt_in += M;
        cnt_out += L;
    }

    MANUALLY_UNROLLED_BACK;
}

uint32_t resampler_t::resample_L9M10_45_5_impl(const cf_t* input,
                                               cf_t* output,
                                               const uint32_t N_new_input_samples) {
    MANUALLY_UNROLLED_FRONT;

    // run manually unrolled loop
    for (uint32_t i = 0; i < full_M; ++i) {
        // clang-format off
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[4], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[5], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[6], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[7], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[8], 5);
        B++; //volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[7], 5);   // no output sample for one input sample
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[0], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[1], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[2], 5);
        volk_32fc_32f_dot_prod_32fc_u((lv_32fc_t*)A++, (const lv_32fc_t*)B++, subfilters[3], 5);
        // clang-format on

        cnt_in += M;
        cnt_out += L;
    }

    MANUALLY_UNROLLED_BACK;
}

}  // namespace dectnrp::phy
