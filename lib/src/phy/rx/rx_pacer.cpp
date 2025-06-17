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

#include "dectnrp/phy/rx/rx_pacer.hpp"

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/rx/rx_pacer_param.hpp"

namespace dectnrp::phy {

rx_pacer_t::rx_pacer_t(const uint32_t nof_antennas_limited_,
                       const radio::buffer_rx_t& buffer_rx_,
                       const uint32_t ant_streams_unit_length_samples_,
                       resampler_t* resampler_)
    : nof_antennas_limited(nof_antennas_limited_),
      ant_streams_length_samples(buffer_rx_.ant_streams_length_samples),
      ant_streams_unit_length_samples(ant_streams_unit_length_samples_),
      buffer_rx(buffer_rx_),
      ant_streams(buffer_rx.get_ant_streams()),
      resampler(resampler_) {
    dectnrp_assert(nof_antennas_limited <= buffer_rx.nof_antennas, "too many antennas");

    // Internally, buffer_rx uses a ring buffer and its size must be a multiple of
    // ant_streams_unit_length_samples.
    dectnrp_assert(ant_streams_length_samples % ant_streams_unit_length_samples == 0,
                   "ant_streams_length_samples not a multiple of ant_streams_unit_length_samples");

    for (uint32_t i = 0; i < nof_antennas_limited; ++i) {
        ant_streams_edge.push_back(srsran_vec_cf_malloc(ant_streams_unit_length_samples));
    }

    ant_streams_offset.resize(nof_antennas_limited);
    localbuffer_offset.resize(nof_antennas_limited);

    // At the transmitter, we usually use L=M=1, L=10 and M=9 or L=40 and M=27. At the receiver,
    // these values are swapped, so L=M=1, L=9 and M=10 or L=27 and M=40.
    dectnrp_assert(resampler->L <= resampler->M,
                   "RX resampler does not keep or decrease sample rate");
    dectnrp_assert(resampler->get_minimum_nof_input_samples() <= ant_streams_unit_length_samples,
                   "input size too short for resampler");
}

rx_pacer_t::~rx_pacer_t() {
    for (auto& elem : ant_streams_edge) {
        free(elem);
    }
}

void rx_pacer_t::filter_single_unit() {
    // convert current global input time to local index in ant_streams
    const uint32_t index = static_cast<uint32_t>(lb_filter->ant_streams_time_64 %
                                                 static_cast<int64_t>(ant_streams_length_samples));

    // how many samples can we read at the right edge of ant_streams?
    const uint32_t right = ant_streams_length_samples - index;

    // where should we write those samples?
    const uint32_t right_offset = lb_filter->cnt_w;

    // enough samples to avoid wrapping?
    if (right >= ant_streams_unit_length_samples) {
        for (uint32_t i = 0; i < nof_antennas_limited; ++i) {
            srsran_vec_cf_copy(&lb_filter->buffer_vec[i][right_offset],
                               &ant_streams[i][index],
                               ant_streams_unit_length_samples);
        }
    }
    // copy from right and left edge
    else {
        const uint32_t left = ant_streams_unit_length_samples - right;
        const uint32_t left_offset = lb_filter->cnt_w + right;

        // copy from ant_streams to localbuffer
        for (uint32_t i = 0; i < nof_antennas_limited; ++i) {
            srsran_vec_cf_copy(
                &lb_filter->buffer_vec[i][right_offset], &ant_streams[i][index], right);
            srsran_vec_cf_copy(&lb_filter->buffer_vec[i][left_offset], &ant_streams[i][0], left);
        }
    }

    lb_filter->cnt_w += ant_streams_unit_length_samples;
}

void rx_pacer_t::resample_single_unit() {
    // convert current global input time to local index in ant_streams
    const uint32_t index = static_cast<uint32_t>(lb_resampler->ant_streams_time_64 %
                                                 static_cast<int64_t>(ant_streams_length_samples));

    // how many samples can we read at the right edge of ant_streams?
    const uint32_t right = ant_streams_length_samples - index;

    // where should we write those samples?
    const uint32_t right_offset = lb_resampler->cnt_w;

    // enough samples to avoid wrapping?
    if (right >= ant_streams_unit_length_samples) {
        // ant_streams_offset points to ant_streams
        for (uint32_t i = 0; i < nof_antennas_limited; ++i) {
            ant_streams_offset[i] = &ant_streams[i][index];
        }
    }
    // wrap into ant_streams_edge and overwrite ant_streams_offset
    else {
        const uint32_t left = ant_streams_unit_length_samples - right;

        // ant_streams_offset points to ant_streams_edge
        for (uint32_t i = 0; i < nof_antennas_limited; ++i) {
            srsran_vec_cf_copy(ant_streams_edge[i], &ant_streams[i][index], right);
            srsran_vec_cf_copy(&ant_streams_edge[i][right], ant_streams[i], left);
            ant_streams_offset[i] = ant_streams_edge[i];
        }
    }

    // output of resampler with correct offset
    for (uint32_t i = 0; i < nof_antennas_limited; ++i) {
        localbuffer_offset[i] = &lb_resampler->buffer_vec[i][right_offset];
    }

    lb_resampler->cnt_w += resampler->resample(
        ant_streams_offset, localbuffer_offset, ant_streams_unit_length_samples);
}

std::vector<cf_t*> rx_pacer_t::get_initialized_localbuffer(
    const localbuffer_choice_t lbc, const uint32_t localbuffer_length_samples_max) {
    // which localbuffer do we initialize?
    if (lbc == localbuffer_choice_t::LOCALBUFFER_FILTER) {
        dectnrp_assert(lb_filter.get() == nullptr, "localbuffer already initialized");

        // must be the next larger multiple of ant_streams_unit_length_samples
        const uint32_t nof_samples =
            common::adt::ceil_divide_integer(localbuffer_length_samples_max,
                                             ant_streams_unit_length_samples) *
            ant_streams_unit_length_samples;

        lb_filter = std::make_unique<localbuffer_t>(nof_antennas_limited, nof_samples);

        return lb_filter->buffer_vec;
    }

    dectnrp_assert(lb_resampler.get() == nullptr, "localbuffer already initialized");

    // how many samples does the resampler output per unit at most?
    const uint32_t resampler_nof_samples_per_unit_max =
        common::adt::ceil_divide_integer(ant_streams_unit_length_samples, resampler->M) *
        resampler->L;

    /* How many units are then required to produce a sufficient amount of samples? We
     * must add +1 because due to the internal history of the resampler, the first unit
     * does not deliver resampler_nof_samples_per_unit_max-many samples.
     */
    const uint32_t resampler_max_nof_units =
        1 + common::adt::ceil_divide_integer(localbuffer_length_samples_max,
                                             resampler_nof_samples_per_unit_max);

    // maximum number of output samples the resampler can gegenerate
    const uint32_t nof_samples = resampler_nof_samples_per_unit_max * resampler_max_nof_units;

    dectnrp_assert(nof_samples >= localbuffer_length_samples_max, "localbuffer too short");

    lb_resampler = std::make_unique<localbuffer_t>(nof_antennas_limited, nof_samples);

    return lb_resampler->buffer_vec;
}

void rx_pacer_t::reset_localbuffer(const localbuffer_choice_t lbc,
                                   const int64_t ant_streams_time_64_) {
    switch (lbc) {
        case localbuffer_choice_t::LOCALBUFFER_FILTER:
            lb_filter->ant_streams_time_64 = ant_streams_time_64_;
            lb_filter->cnt_w = 0;
            break;

        case localbuffer_choice_t::LOCALBUFFER_RESAMPLE:
            resampler->reset();
            lb_resampler->ant_streams_time_64 = ant_streams_time_64_;
            lb_resampler->cnt_w = 0;
            break;
    }
}

#ifdef ENABLE_ASSERT
void rx_pacer_t::assert_time_lag(const int64_t time_to_check_for_lag_64) const {
    // poll current time
    const int64_t now_64 = buffer_rx.get_rx_time_passed();

    dectnrp_assert(time_to_check_for_lag_64 <= now_64,
                   "time lag can only be checked for past times");

    // has enough time past since the start time to even check the time lag?
    if (search_time_start_64 + static_cast<int64_t>(buffer_rx.samp_rate) *
                                   int64_t{RX_PACER_LAG_ASSERT_AFTER_START_TIME_MS} /
                                   int64_t{1000} <=
        now_64) {
        // if not, return immediately
        return;
    }

    dectnrp_assert(now_64 - time_to_check_for_lag_64 <=
                       (static_cast<int64_t>(ant_streams_length_samples) *
                        RX_PACER_LAG_ASSERT_TENTHS_RX_ANT_STREAMS_LENGTH / 10),
                   "time lag too large");
}
#endif

void rx_pacer_t::wait_until_nto(const int64_t global_time_64) const {
    buffer_rx.wait_until_nto(global_time_64);
}

uint32_t rx_pacer_t::filter_until_nto(const uint32_t cnt_w_min) {
    // immediately return if we already have enough samples
    if (lb_filter->cnt_w >= cnt_w_min) {
        return lb_filter->cnt_w;
    }

    int64_t now_64 = buffer_rx.get_rx_time_passed();

    /* We work in small steps until the correct amount of samples is written. In order to execute
     * each of these steps, a target time must be reached.
     */
    int64_t target_64 = 0;

    // keep filtering until we have enough samples
    while (lb_filter->cnt_w < cnt_w_min) {
        // what time do we have to reach to be able to filter one full unit?
        target_64 =
            lb_filter->ant_streams_time_64 + static_cast<int64_t>(ant_streams_unit_length_samples);

        // if we haven't reached that time yet, we have to wait
        if (now_64 < target_64) {
            now_64 = buffer_rx.wait_until_nto(target_64);
        }

        filter_single_unit();

        lb_filter->ant_streams_time_64 = target_64;
    }

    return lb_filter->cnt_w;
}

void rx_pacer_t::rewind_localbuffer_resample_cnt_w() { lb_resampler->cnt_w = 0; }

uint32_t rx_pacer_t::resample_until_nto(const uint32_t cnt_w_min) {
    // immediately return if we already have enough samples
    if (lb_resampler->cnt_w >= cnt_w_min) {
        return lb_resampler->cnt_w;
    }

    int64_t now_64 = buffer_rx.get_rx_time_passed();

    /* We work in small steps until the correct amount of samples is written. In order to execute
     * each of these steps, a target time must be reached.
     */
    int64_t target_64 = 0;

    // keep resampling until we have enough samples
    while (lb_resampler->cnt_w < cnt_w_min) {
        // what time do we have to reach to be able to resample one full unit?
        target_64 = lb_resampler->ant_streams_time_64 +
                    static_cast<int64_t>(ant_streams_unit_length_samples);

        // if we haven't reached that time yet, we have to wait
        if (now_64 < target_64) {
            now_64 = buffer_rx.wait_until_nto(target_64);
        }

        resample_single_unit();

        lb_resampler->ant_streams_time_64 = target_64;
    }

    return lb_resampler->cnt_w;
}

uint32_t rx_pacer_t::convert_length_global_to_resampled(const uint32_t global_length) const {
    double res = static_cast<double>(global_length);

    res /= static_cast<double>(resampler->M);
    res *= static_cast<double>(resampler->L);

    return static_cast<uint32_t>(std::round(res));
}

uint32_t rx_pacer_t::convert_length_resampled_to_global(const uint32_t resampled_length) const {
    double res = static_cast<double>(resampled_length);

    res *= static_cast<double>(resampler->M);
    res /= static_cast<double>(resampler->L);

    return static_cast<uint32_t>(std::round(res));
}

uint32_t rx_pacer_t::convert_time_global_to_resampled(const int64_t global_time_64,
                                                      const int64_t global_time_offset_64) const {
    return convert_length_global_to_resampled(
        static_cast<uint32_t>(global_time_64 - global_time_offset_64));
}

int64_t rx_pacer_t::convert_time_resampled_to_global(const uint32_t resampled_time,
                                                     const int64_t global_time_offset_64) const {
    return static_cast<int64_t>(convert_length_resampled_to_global(resampled_time)) +
           global_time_offset_64;
}

}  // namespace dectnrp::phy
