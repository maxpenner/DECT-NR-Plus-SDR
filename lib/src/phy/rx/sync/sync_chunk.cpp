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

#include "dectnrp/phy/rx/sync/sync_chunk.hpp"

#include <algorithm>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"
#include "dectnrp/sections_part3/stf.hpp"

namespace dectnrp::phy {

sync_chunk_t::sync_chunk_t(const radio::buffer_rx_t& buffer_rx_,
                           const worker_pool_config_t& worker_pool_config_,
                           const uint32_t chunk_length_samples_,
                           const uint32_t chunk_stride_samples_,
                           const uint32_t chunk_offset_samples_,
                           const uint32_t ant_streams_unit_length_samples_)
    // clang-format off
    : rx_pacer_t(std::min(buffer_rx_.nof_antennas, RX_SYNC_PARAM_AUTOCORRELATOR_ANTENNA_LIMIT),
                 buffer_rx_,
                 ant_streams_unit_length_samples_,
                 new resampler_t(std::min(buffer_rx_.nof_antennas, RX_SYNC_PARAM_AUTOCORRELATOR_ANTENNA_LIMIT),
                                 // at the receiver L and M are switched, see tx.cpp
                                 worker_pool_config_.resampler_param.M,
                                 worker_pool_config_.resampler_param.L,
                                 resampler_param_t::f_pass_norm[resampler_param_t::user_t::SYNC][worker_pool_config_.os_min],
                                 resampler_param_t::f_stop_norm[resampler_param_t::user_t::SYNC][worker_pool_config_.os_min],
                                 resampler_param_t::PASSBAND_RIPPLE_DONT_CARE,
                                 resampler_param_t::f_stop_att_dB[resampler_param_t::user_t::SYNC][worker_pool_config_.os_min])),
      // clang-format on
      chunk_length_samples(chunk_length_samples_),
      chunk_stride_samples(chunk_stride_samples_),
      chunk_offset_samples(chunk_offset_samples_),

      u_max(worker_pool_config_.radio_device_class.u_min),
      stf_nof_pattern(sp3::stf_t::get_N_stf_pattern(u_max)),

      bos_fac(worker_pool_config_.radio_device_class.b_min * worker_pool_config_.os_min),
      stf_bos_length_samples(sp3::stf_t::get_N_samples_stf(u_max) * bos_fac),
      stf_bos_pattern_length_samples(stf_bos_length_samples / stf_nof_pattern),

      A(chunk_length_samples / worker_pool_config_.resampler_param.L *
        worker_pool_config_.resampler_param.M),
      B(static_cast<uint32_t>(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_OVERLAP_LENGTH_IN_STFS_DP *
                              static_cast<double>(stf_bos_length_samples))),
      C(stf_bos_pattern_length_samples),
      D(static_cast<uint32_t>(RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MAX_SEARCH_LENGTH_IN_STFS_DP *
                              static_cast<double>(stf_bos_length_samples))) {
    // initialize buffer where samples after resampling will be written to
    const std::vector<cf_t*> localbuffer_resample = get_initialized_localbuffer(
        rx_pacer_t::localbuffer_choice_t::LOCALBUFFER_RESAMPLE, A + B + C + D);

    autocorrelator_detection =
        std::make_unique<autocorrelator_detection_t>(localbuffer_resample,
                                                     nof_antennas_limited,
                                                     stf_bos_length_samples,
                                                     stf_bos_pattern_length_samples,
                                                     A + B,
                                                     worker_pool_config_.get_dect_samp_rate_max());

    autocorrelator_peak =
        std::make_unique<autocorrelator_peak_t>(localbuffer_resample,
                                                worker_pool_config_.radio_device_class.u_min,
                                                worker_pool_config_.radio_device_class.b_min,
                                                nof_antennas_limited,
                                                bos_fac,
                                                stf_bos_length_samples,
                                                stf_bos_pattern_length_samples,
                                                D);

    /* The exact number of samples is calculated in the crosscorrelator. This, however, is a very
     * precise estimate of the number of samples the crosscorrelator will requires at most.
     */
    const uint32_t crosscorrelator_localbuffer_length_samples =
        (RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_LEFT_SAMPLES * bos_fac +
         RX_SYNC_PARAM_CROSSCORRELATOR_SEARCH_RIGHT_SAMPLES * bos_fac + stf_bos_length_samples +
         1) *
        worker_pool_config_.resampler_param.L / worker_pool_config_.resampler_param.M;

    const std::vector<cf_t*> localbuffer_filter =
        get_initialized_localbuffer(rx_pacer_t::localbuffer_choice_t::LOCALBUFFER_FILTER,
                                    crosscorrelator_localbuffer_length_samples);

    crosscorrelator =
        std::make_unique<crosscorrelator_t>(localbuffer_filter,
                                            u_max,
                                            worker_pool_config_.radio_device_class.b_min,
                                            worker_pool_config_.os_min,
                                            buffer_rx_.nof_antennas,
                                            nof_antennas_limited,
                                            worker_pool_config_.resampler_param);

    // assert we allocated enough for the crosscorrelator, but not way too much
    dectnrp_assert(0 <= (static_cast<int32_t>(crosscorrelator_localbuffer_length_samples) -
                         static_cast<int32_t>(crosscorrelator->get_nof_samples_required())),
                   "too small");
    dectnrp_assert((static_cast<int32_t>(crosscorrelator_localbuffer_length_samples) -
                    static_cast<int32_t>(crosscorrelator->get_nof_samples_required())) <= 5,
                   "too large");
}

void sync_chunk_t::wait_for_first_chunk_nto(const int64_t search_time_start_64_) {
#ifdef ENABLE_ASSERT
    search_time_start_64 = search_time_start_64_;
#endif

    chunk_time_start_64 = search_time_start_64_ -
                          (search_time_start_64_ % static_cast<int64_t>(chunk_stride_samples));

    chunk_time_start_64 += static_cast<int64_t>(chunk_offset_samples);

    const int64_t chunk_time_end_64 =
        chunk_time_start_64 + static_cast<int64_t>(chunk_length_samples);

    // start time can be later than chunk end, in that case directly jump to next chunk
    if (chunk_time_end_64 <= search_time_start_64_) {
        set_next_chunk();
    }

    wait_for_chunk_nto();
}

std::optional<sync_report_t> sync_chunk_t::search() {
    // create a fresh sync_report, by default indicating that no packet was found
    sync_report_t sync_report(nof_antennas_limited);

    dectnrp_assert(!sync_report.coarse_peak_array.has_any_larger(0.0f), "antenna already valid");

    // when being called, we might have processed the entire chunk, in that case jump to next
    // chunk
    if (is_chunk_completely_processed()) {
        set_next_chunk();
        wait_for_chunk_nto();
    }

    // start or continue processing until autocorrelator has covered entire search range
    while (autocorrelator_detection->get_localbuffer_cnt_r() <
           autocorrelator_detection->search_length_samples) {
        // let pacer resample as much as is required for next step
        uint32_t localbuffer_cnt_w =
            resample_until_nto(autocorrelator_detection->get_nof_samples_required());

        if (autocorrelator_detection->search_by_correlation(localbuffer_cnt_w, sync_report)) {
            ++stats.detections;

            /// \todo add u detection to correlator
            // For now we set the value of u to the maximum of the radio device class. Ideally, the
            // current of value of u would be provided by the detector.
            sync_report.u = u_max;

            dectnrp_assert(
                stf_bos_length_samples <= sync_report.detection_time_with_jump_back_local,
                "detection time too early");

            // configure coarse peak search ...
            autocorrelator_peak->set_initial_state(sync_report.detection_time_with_jump_back_local);

            // ... and run it
            while (1) {
                localbuffer_cnt_w =
                    resample_until_nto(autocorrelator_peak->get_nof_samples_required());

                if (autocorrelator_peak->search_by_correlation(localbuffer_cnt_w, sync_report)) {
                    break;
                }
            }

            dectnrp_assert(autocorrelator_peak->get_localbuffer_cnt_r() <=
                               autocorrelator_detection->search_length_samples +
                                   stf_bos_pattern_length_samples + stf_bos_length_samples,
                           "autocorrelator_peak read more samples than allowed");

            /* We detected a metric above the threshold and the autocorrelator_peak searched for a
             * valid peak after the coarse detection. Here, we check whether it found a peak. If so,
             * we most likely detected a rising edge.
             */
            if (sync_report.coarse_peak_array.has_any_larger(0.0f)) {
                ++stats.coarse_peaks;

                autocorrelator_detection->skip_after_peak(sync_report.coarse_peak_time_local);

                // coarse peak time local is relative to the chunk start, convert to global time
                sync_report.coarse_peak_time_64 = convert_time_resampled_to_global(
                    sync_report.coarse_peak_time_local, chunk_time_start_64);

#ifdef RX_SYNC_PARAM_DBG_COARSE_SYNC_PEAK_FORCED_TO_TIME_MULTIPLE
                // This is debug functionality to simulate perfect synchronization, see sync_param.h
                // for details.

                const int64_t sync_error_left =
                    sync_report.coarse_peak_time_64 %
                    RX_SYNC_PARAM_DBG_COARSE_SYNC_PEAK_FORCED_TO_TIME_MULTIPLE;

                const int64_t sync_error_right =
                    RX_SYNC_PARAM_DBG_COARSE_SYNC_PEAK_FORCED_TO_TIME_MULTIPLE - sync_error_left;

                // check which error is smaller and ...
                if (sync_error_left <= sync_error_right) {
                    // ... either jump to the left ...
                    sync_report.coarse_peak_time_64 -= sync_error_left;
                } else {
                    // ... or to the right
                    sync_report.coarse_peak_time_64 += sync_error_right;
                }

                // overwrite local peak index accordingly for the fine peak search
                sync_report.coarse_peak_time_local = convert_time_global_to_resampled(
                    sync_report.coarse_peak_time_64, chunk_time_start_64);
#endif

                // reset crosscorrelator
                crosscorrelator->set_initial_state();

                // reset and rewind, tell pacer when to start copying samples from
                reset_localbuffer(rx_pacer_t::localbuffer_choice_t::LOCALBUFFER_FILTER,
                                  sync_report.coarse_peak_time_64 -
                                      static_cast<int64_t>(crosscorrelator->search_length_l));

                /* Let the pacer copy samples until crosscorrelator has enough input samples. This
                 * is done is a single step, as there is no benefit in splitting it up. The full STF
                 * has already been received at RX since we found the coarse peak, and the right
                 * search area is very small.
                 */
                const uint32_t localbuffer_cnt =
                    filter_until_nto(crosscorrelator->get_nof_samples_required());

                // let the crosscorrelator determine b, integer CFO and the fine peak on local time
                crosscorrelator->search_by_correlation(localbuffer_cnt, sync_report);

                // convert local time of fine peak to global time
                sync_report.fine_peak_time_64 =
                    sync_report.coarse_peak_time_64 -
                    static_cast<int64_t>(crosscorrelator->search_length_l) +
                    static_cast<int64_t>(sync_report.fine_peak_time_local);

                return sync_report;
            } else {
                // reset all values of sync_report
                sync_report = sync_report_t(nof_antennas_limited);
            }
        }
    }

    ++stats.chunk_processed_without_detection_at_the_end;

    return std::nullopt;
}

bool sync_chunk_t::is_chunk_completely_processed() {
    return autocorrelator_detection->search_length_samples <=
           autocorrelator_detection->get_localbuffer_cnt_r();
}

int64_t sync_chunk_t::get_chunk_time_start() const { return chunk_time_start_64; }

int64_t sync_chunk_t::get_chunk_time_end() const {
    return chunk_time_start_64 + static_cast<int64_t>(chunk_length_samples - 1);
}

void sync_chunk_t::set_next_chunk() {
#ifdef ENABLE_ASSERT
    check_time_lag(get_chunk_time_end());
#endif

    chunk_time_start_64 += static_cast<int64_t>(chunk_stride_samples);
}

void sync_chunk_t::wait_for_chunk_nto() {
    ++stats.waitings_for_chunk;

    // reset before wait_until_nto()
    reset_localbuffer(rx_pacer_t::localbuffer_choice_t::LOCALBUFFER_RESAMPLE, chunk_time_start_64);
    autocorrelator_detection->reset();

    wait_until_nto(chunk_time_start_64);

    autocorrelator_detection->set_power_of_first_stf_pattern(
        resample_until_nto(stf_bos_pattern_length_samples));
}

}  // namespace dectnrp::phy
