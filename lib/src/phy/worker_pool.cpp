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

#include "dectnrp/phy/worker_pool.hpp"

#include <string>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/threads.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"
#include "dectnrp/sections_part3/stf.hpp"

namespace dectnrp::phy {

worker_pool_t::worker_pool_t(const worker_pool_config_t& worker_pool_config_,
                             radio::hw_t& hw_,
                             phy_radio_t& phy_radio_)
    : layer_unit_t(worker_pool_config_.json_log_key, worker_pool_config_.id),
      worker_pool_config(worker_pool_config_) {
    keep_running.store(false, std::memory_order_release);

    job_queue = std::make_unique<job_queue_t>(id, worker_pool_config.nof_jobs);

    worker_config_t worker_config(0, keep_running, hw_, *job_queue.get(), worker_pool_config);

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
    if (worker_pool_config.json_export_length > 0) {
        dectnrp_assert(
            worker_pool_config.json_export_length >= JSON_export_minimum_number_of_packets,
            "too few JSONs");

        dectnrp_assert(worker_pool_config.threads_core_prio_config_tx_rx_vec.size() > 1,
                       "exporting JSON requires at least two threads");

        json_export = std::make_unique<common::json_export_t>(
            worker_pool_config.json_export_length,
            "worker_pool_" + common::json_export_t::get_number_with_leading_zeros(id, 4) + "_",
            "packet_");
    }
#endif

    // create TX/RX workers
    for (uint32_t worker_id = 0;
         worker_id < worker_pool_config.threads_core_prio_config_tx_rx_vec.size();
         ++worker_id) {
        // update ID
        worker_config.id = worker_id;

        // create
        worker_tx_rx_vec.push_back(std::make_unique<worker_tx_rx_t>(
            worker_config, irregular, phy_radio_, json_export.get()));
    }

    check_sync_param();
    check_sync_timing();

    // baton requires number of sync workers
    baton = std::make_unique<baton_t>(worker_pool_config.threads_core_prio_config_sync_vec.size(),
                                      get_sync_time_unique_limit(),
                                      worker_pool_config.rx_job_regular_period);

    // create sync workers
    for (uint32_t worker_id = 0;
         worker_id < worker_pool_config.threads_core_prio_config_sync_vec.size();
         ++worker_id) {
        // update ID
        worker_config.id = worker_id;

        // create
        worker_sync_vec.push_back(
            std::make_unique<worker_sync_t>(worker_config, *baton.get(), irregular));
    }
}

void worker_pool_t::configure_tpoint_calls(upper::tpoint_t* tpoint_,
                                           std::shared_ptr<token_t> token_,
                                           const uint32_t token_call_id_) {
    dectnrp_assert(tpoint_ != nullptr, "invalid tpoint");
    dectnrp_assert(token_.get() != nullptr, "invalid token");
    dectnrp_assert(token_call_id_ < limits::max_nof_radio_phy_pairs_one_tpoint, "ID too large");

    for (auto& elem : worker_tx_rx_vec) {
        elem->tpoint = tpoint_;
        elem->token = token_;
        elem->token_call_id = token_call_id_;
    }

    baton->set_tpoint_to_notify(tpoint_, token_);
}

void worker_pool_t::add_network_id(const uint32_t network_id) {
    for (auto& elem : worker_tx_rx_vec) {
        elem->tx->add_new_network_id(network_id);
        elem->rx_synced->add_new_network_id(network_id);
    }
}

void worker_pool_t::start_threads_and_get_ready_to_process_iq_samples() {
    // assert
    for (auto& elem : worker_tx_rx_vec) {
        dectnrp_assert(elem->tpoint != nullptr, "tpoint pointer not initialized.");
    }

    dectnrp_assert(!keep_running.load(std::memory_order_acquire), "keep_running already true");

    // tell threads they can start working
    keep_running.store(true, std::memory_order_release);

    // spawn workers for TX/RX first, they consume jobs
    for (uint32_t i = 0; i < worker_tx_rx_vec.size(); ++i) {
        if (!common::threads_new_rt_mask_custom(
                &worker_tx_rx_vec[i]->work_thread,
                &work_spawn,
                worker_tx_rx_vec[i].get(),
                worker_pool_config.threads_core_prio_config_tx_rx_vec[i])) {
            dectnrp_assert_failure("Worker pool {} unable to start TX/RX work thread.", id);
        }

        log_line(std::string("Thread Worker TX/RX " +
                             common::get_thread_properties(
                                 worker_tx_rx_vec[i]->work_thread,
                                 worker_pool_config.threads_core_prio_config_tx_rx_vec[i])));
    }

    // spawn workers for sync second, they produce jobs
    for (uint32_t i = 0; i < worker_sync_vec.size(); ++i) {
        if (!common::threads_new_rt_mask_custom(
                &worker_sync_vec[i]->work_thread,
                &work_spawn,
                worker_sync_vec[i].get(),
                worker_pool_config.threads_core_prio_config_sync_vec[i])) {
            dectnrp_assert_failure("Worker pool {} unable to start sync work thread.", id);
        }

        log_line(std::string("Thread Worker Sync " +
                             common::get_thread_properties(
                                 worker_sync_vec[i]->work_thread,
                                 worker_pool_config.threads_core_prio_config_sync_vec[i])));
    }

    /* The synchronization configuration determines how often a termination point firmware is called
     * per second, i.e. the number of regular calls. Since that number if quite useful, we write it
     * directly into the log file.
     */
    const uint32_t regular_calls_per_second =
        constants::u8_subslots_per_sec / worker_pool_config.rx_chunk_length_u8subslot;
    const float regular_calls_span_us = 1e6 / static_cast<float>(regular_calls_per_second);

    log_line("regular_calls_per_second " + std::to_string(regular_calls_per_second) +
             " regular_calls_span_us " + std::to_string(regular_calls_span_us));

    log_lines(job_queue->report_start());

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
    if (json_export.get() != nullptr) {
        log_lines(json_export->report_start());
    }
#endif

    for (const auto& elem : worker_tx_rx_vec) {
        log_lines(elem->report_start());
    }

    for (const auto& elem : worker_sync_vec) {
        log_lines(elem->report_start());
    }
}

void worker_pool_t::work_stop() {
    dectnrp_assert(keep_running.load(std::memory_order_acquire), "keep_running already false");

    // this variable will be read in all workers and leads to them leaving their loops
    keep_running.store(false, std::memory_order_release);

    // stop job producers first
    for (auto& elem : worker_sync_vec) {
        pthread_join(elem->work_thread, NULL);
        log_line(std::string("Thread Worker TX/RX " + std::to_string(elem->id)));
    }

    // stop job consumers second
    for (auto& elem : worker_tx_rx_vec) {
        pthread_join(elem->work_thread, NULL);
        log_line(std::string("Thread Worker Sync " + std::to_string(elem->id)));
    }

    log_lines(job_queue->report_stop());

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
    if (json_export.get() != nullptr) {
        log_lines(json_export->report_stop());
    }
#endif

    for (auto& elem : worker_tx_rx_vec) {
        log_lines(elem->report_stop());
    }

    for (auto& elem : worker_sync_vec) {
        log_lines(elem->report_stop());
    }
}

void worker_pool_t::check_sync_param() const {
    // get long cover sequence for u=2
    const auto cs = sp3::stf_t::get_cover_sequence(2);

    dectnrp_assert(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_FRONT_STEPS >=
                       RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER / 2,
                   "front minimum length is half a pattern");
    dectnrp_assert(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_RMS_BACK_STEPS >=
                       RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER / 2,
                   "back minimum length is half a pattern");

    // are we using a non-trivial cover sequence?
    if (std::any_of(cs.begin(), cs.end(), [](const float val) { return val != 1.0f; })) {
        dectnrp_assert(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_STEP_DIVIDER > 2,
                       "cover sequence requires smaller step divider");
        dectnrp_assert(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_THRESHOLD_MIN_SP <= 0.2f,
                       "cover sequence requires smaller metric threshold");
        dectnrp_assert(RX_SYNC_PARAM_AUTOCORRELATOR_DETECTION_METRIC_STREAK == 1,
                       "cover sequence requires a metric streak of 1");
        dectnrp_assert(RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_LEFT ==
                           RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_RIGHT,
                       "cover sequence requires symmetric smoothing");
        dectnrp_assert(RX_SYNC_PARAM_AUTOCORRELATOR_PEAK_MOVMEAN_SMOOTH_LEFT <= 1,
                       "cover sequence requires small smoothing range");
    }
}

void worker_pool_t::check_sync_timing() const {
    // clang-format off
    /* What is the relationship between the RX buffer, slots, chunks, sync workers and u=8-subslots?
     * 
     *  Example:    RX buffer of 10 milliseconds, can be shorter or longer
     *              24 slots
     *              Each slots contains 16 u=8-subslots (this is a fixed DECT NR+ property)
     *              Each chunk contains 8 u=8-subslots
     *              Chunks are split between 2 workers
     *              Within the workers, the resampler is fed with units, their size is a multiple of u=8-subslots.
     * 
     *                |                                                           RX buffer of 24 slots length                                                                    |
     *                |                                                                                                                                                           |
     *                |                    Slot 0                     |                    Slot 1                     |           |                    Slot 23                    |
     *                |                                               |                                               |           |                                               |
     *                |       Chunk  0        |       Chunk  1        |       Chunk  2        |       Chunk  3        |           |       Chunk 46        |       Chunk 47        |
     *                |       Worker 0        |       Worker 1        |       Worker 0        |       Worker 1        |           |       Worker 0        |       Worker 1        |
     *                |                       |                       |                       |                       |           |                       |                       |
     *    ...|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__  ...  __|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__|__...
     *   u=8-Subslot   1  2  3  4  5  6  7  8  1  2  3  4  5  6  7  8  1  2  3  4  5  6  7  8  1  2  3  4  5  6  7  8  1        8  1  2  3  4  5  6  7  8  1  2  3  4  5  6  7  8  1  2
     */
    // clang-format on

    dectnrp_assert(
        worker_pool_config.resampler_param.hw_samp_rate % constants::u8_subslots_per_sec == 0,
        "HW sample rate not a multiple of the number of u=8-subslots 2400*16=38400.");

    const uint32_t u8subslot_length_samples =
        worker_pool_config.resampler_param.hw_samp_rate / constants::u8_subslots_per_sec;

    /* To have unambiguous, non-fractional mapping of samples before and after resampling in
     * time domain, lengths should be a multiple of L. Since all lengths are a multiples of
     * u=8-subslots, it's enough to check that length.
     */
    dectnrp_assert(u8subslot_length_samples % worker_pool_config.resampler_param.L == 0,
                   "Length of u=8-subslot not a multiple of the tx upscaling factor L.");

    // RX buffer length is given in slots, each slot contains 16 u=8-subslots
    const uint32_t nof_subslot_u_8_in_rx_ant_streams =
        worker_pool_config.rx_ant_streams_length_slots * constants::u8_subslots_per_slot;

    // how many sync worker do we have?
    const uint32_t nof_workers = worker_pool_config.threads_core_prio_config_sync_vec.size();

    dectnrp_assert(nof_subslot_u_8_in_rx_ant_streams %
                           (nof_workers * worker_pool_config.rx_chunk_length_u8subslot) ==
                       0,
                   "RX ring buffer not evenly splittable between chunks");
}

int64_t worker_pool_t::get_sync_time_unique_limit() const {
    /* In a DECT NR+ packet transmission, the reasonable minimum time difference between two packet
     * synchronization times is the length of the shortest possible STF, otherwise STFs overlap.
     * With DF and GI taken into consideration, the reasonable minimum time difference between two
     * packets becomes even larger. However, if we synchronize the same packet twice in different
     * sync_worker_t instances, the time difference between both synchronization times is much
     * smaller. Thus, we can easily distinguish these both cases.
     *
     * For a synchronization time to be considered unique, it must be later than the predecessor and
     * the distance to the predecessor must be larger than the limit defined here.
     */

    // load parameters for u_max which has the shortest STF
    const uint32_t u_max = worker_pool_config.radio_device_class.u_min;
    const uint32_t stf_length_samples = sp3::stf_t::get_N_samples_stf(u_max);
    const uint32_t stf_nof_pattern = sp3::stf_t::get_N_stf_pattern(u_max);

    // determine the STF pattern length for u_max, b_max and oversampling (bos)
    const uint32_t bos_fac =
        worker_pool_config.radio_device_class.b_min * worker_pool_config.os_min;
    const uint32_t stf_bos_length_samples = stf_length_samples * bos_fac;
    const uint32_t stf_bos_pattern_length_sample = stf_bos_length_samples / stf_nof_pattern;

    return static_cast<int64_t>(static_cast<double>(stf_bos_pattern_length_sample) *
                                RX_SYNC_PARAM_SYNC_TIME_UNIQUE_LIMIT_IN_STF_PATTERNS_DP);
}

}  // namespace dectnrp::phy
