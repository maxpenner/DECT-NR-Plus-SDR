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

#include "dectnrp/phy/pool/worker_sync.hpp"

#include <array>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/rx/sync/time_report.hpp"

namespace dectnrp::phy {

worker_sync_t::worker_sync_t(worker_config_t& worker_config, baton_t& baton_)
    : worker_t(worker_config),
      baton(baton_) {
    /* The length of a chunk is the same for every worker_sync_t instance, but their chunks are
     * interleaved by fixed offsets.
     */
    const uint32_t u8subslot_length_samples =
        worker_pool_config.resampler_param.hw_samp_rate / constants::u8_subslots_per_sec;
    const uint32_t chunk_length_samples =
        u8subslot_length_samples * worker_pool_config.rx_chunk_length_u8subslot;

    sync_chunk = std::make_unique<sync_chunk_t>(
        buffer_rx,
        worker_pool_config,
        chunk_length_samples,
        chunk_length_samples * worker_pool_config.threads_core_prio_config_sync_vec.size(),
        chunk_length_samples * id,
        u8subslot_length_samples * worker_pool_config.rx_chunk_unit_length_u8subslot);
}

void worker_sync_t::work() {
    warmup();

    int64_t now_64 = buffer_rx.get_rx_time_passed();

    // we don't want synchronization to start right away, but some time in the near future
    now_64 += static_cast<int64_t>(buffer_rx.samp_rate) *
              static_cast<int64_t>(RX_SYNC_PARAM_SYNCHRONIZATION_START_TIME_ADVANCE_MS) /
              static_cast<int64_t>(1000);

    /* Furthermore, we always want worker_sync_t instance with ID 0 to be the first one to hold the
     * baton and to start with its respective chunk, so we also make the potential synchronization
     * start time the next multiple of the buffer length.
     */
    now_64 = common::adt::multiple_geq(now_64,
                                       static_cast<int64_t>(buffer_rx.ant_streams_length_samples));

    /* The current value of now_64 is only a potential start time for synchronization determined in
     * this instance of worker_sync_t. However, we must ensure that all instances of worker_sync_t
     * agree upon one common time where synchronization starts. They do so by registering at the
     * baton with their value of now_64 as a suggestion. The largest value of now_64 wins, and the
     * last instance to register triggers the other instances.
     *
     * Once a one common value of now_64 has been found, the baton will also call the respective
     * termination point and its function work_start_imminent() to tell the firmware at what exact
     * time synchronization is about to start.
     */
    const int64_t start_time_64 = baton.register_and_wait_for_others_nto(now_64);

    dectnrp_assert(start_time_64 % static_cast<int64_t>(buffer_rx.ant_streams_length_samples) == 0,
                   "invalid synchronization start time");

    dectnrp_assert(now_64 <= start_time_64, "invalid synchronization start time");

    // set starting point for synchronization and wait for it
    sync_chunk->wait_for_first_chunk_nto(start_time_64);

    dectnrp_assert(1 <= RX_SYNC_PARAM_MAX_NOF_BUFFERABLE_SYNC_BEFORE_ACQUIRING_BATON,
                   "at least one packet must be bufferable");

    // preallocate the maximum number of instances of sync_report_t that we can buffer
    std::array<sync_report_t, RX_SYNC_PARAM_MAX_NOF_BUFFERABLE_SYNC_BEFORE_ACQUIRING_BATON>
        sync_report_arr;

    // we also need a counter for the number of sync_report_t we are currently buffering
    uint32_t sync_report_arr_cnt = 0;

    // working instance for the search result
    decltype(sync_chunk->search()) sync_report_opt;

    // work loop
    while (keep_running.load(std::memory_order_acquire)) {
        /* At this point, we know that keep_running is true and we are expected to continue our
         * search for packets. Furthermore, we know that we are the start of a new chunk. So, first
         * we reset the counter for buffered instances of sync_report_t.
         */
        sync_report_arr_cnt = 0;

        /* The preceding instance of worker_sync_t has not necessarily passed on the baton yet, but
         * we start processing the chunk anyway. Only once we either found a packet or reached the
         * end of the chunk, we have to bother checking for the baton.
         */
        while (1) {
            // start/continue search for packets
            sync_report_opt = sync_chunk->search();

            // did we find a packet?
            if (sync_report_opt.has_value()) {
                // has the baton been passed on in the meantime?
                if (baton.is_id_holder_the_same(id)) {
                    break;
                }
                // baton has not been passed on yet
                else {
                    // can we buffer another sync_report?
                    if (sync_report_arr_cnt <
                        RX_SYNC_PARAM_MAX_NOF_BUFFERABLE_SYNC_BEFORE_ACQUIRING_BATON) {
                        // buffer the packet and increment counter
                        sync_report_arr.at(sync_report_arr_cnt) = sync_report_opt.value();
                        ++sync_report_arr_cnt;
                    }
                    // we don't hold the baton yet, but we also can't buffer another packet
                    else {
                        break;
                    }
                }
            }
            // search returned without having found a packet, thus we must have reached the end of
            // the chunk
            else {
                break;
            }
        }

        // we now must acquire the baton and ...
        while (!baton.wait_to(id)) {
            // ... only stop waiting for it if the external exit condition is met
            if (!keep_running.load(std::memory_order_acquire)) {
                return;
            }
        }

        // we have acquired the baton, so now put any buffered packets into the job queue
        /// \todo lock and unlock job_queue only once?
        for (uint32_t i = 0; i < sync_report_arr_cnt; ++i) {
            enqueue_job_nto(sync_report_arr[i]);
        }

        /* At this point, we made sure we hold the baton and we enqueued any buffered sync_reports.
         * Now we process the rest of the chunk (actually, we might be done already, in which case
         * the loop is exited immediately).
         */
        while (1) {
            if (sync_report_opt.has_value()) {
                enqueue_job_nto(sync_report_opt.value());
            }

            if (sync_chunk->is_chunk_completely_processed()) {
                break;
            }

            sync_report_opt.reset();

            // continue search
            sync_report_opt = sync_chunk->search();
        };

        // we are done with the chunk
        if (baton.is_job_regular_due()) {
            /* We need to generate a regular job which requires a barrier time as an argument. This
             * time is the latest time for which we can guarantee that no more packets will be found
             * before it. This is either the end of the chunk without the overlap area, or the
             * latest unique packet time.
             */
            const time_report_t time_report(sync_chunk->get_chunk_time_end(),
                                            baton.get_sync_time_last());

            // put job into the queue
            job_queue.enqueue_nto(job_t(std::move(time_report)));

            ++stats.job_regular;
        }

#ifdef ASSERT_ENABLED
        // don't check when called for the first time, indicated by baton.chunk_time_end_64 being
        // set to numeric maximum
        if (baton.chunk_time_end_64 < std::numeric_limits<int64_t>::max()) {
            dectnrp_assert(sync_chunk->get_chunk_time_start() - baton.chunk_time_end_64 == 1,
                           "time not continuous");
        }

        // overwrite for next worker
        baton.chunk_time_end_64 = sync_chunk->get_chunk_time_end();
#endif
        // trigger next worker_sync_t instance
        baton.pass_on(id);
    }
}

std::vector<std::string> worker_sync_t::report_start() const {
    std::vector<std::string> lines;

    std::string str("Worker Sync " + std::to_string(id));

    // components

    str.append(" Chunk Length " + std::to_string(sync_chunk->chunk_length_samples));
    str.append(" Stride " + std::to_string(sync_chunk->chunk_stride_samples));
    str.append(" Offset " + std::to_string(sync_chunk->chunk_offset_samples));

    lines.push_back(str);

    return lines;
}

std::vector<std::string> worker_sync_t::report_stop() const {
    std::vector<std::string> lines;

    std::string str("Worker Sync " + std::to_string(id));
    str.append(" job_regular " + std::to_string(stats.job_regular));
    str.append(" job_packet " + std::to_string(stats.job_packet));
    str.append(" job_packet_not_unique " + std::to_string(stats.job_packet_not_unique));
    str.append(" job_packet_no_slot " + std::to_string(stats.job_packet_no_slot));

    // components

    auto sc_stats = sync_chunk->get_stats();

    str.append(" Sync Chunk");
    str.append(" waitings_for_chunk " + std::to_string(sc_stats.waitings_for_chunk));
    str.append(" detections " + std::to_string(sc_stats.detections));
    str.append(" coarse_peaks " + std::to_string(sc_stats.coarse_peaks));
    str.append(" chunk_processed_without_detection_at_the_end " +
               std::to_string(sc_stats.chunk_processed_without_detection_at_the_end));

    lines.push_back(str);

    return lines;
}

void worker_sync_t::warmup() {
    // wait until the hardware has started streaming (minimum time 1), returns current hw time
    int64_t now_64 = buffer_rx.wait_until_nto(1);

    // get next multiple of the buffer length
    now_64 = common::adt::multiple_geq(now_64,
                                       static_cast<int64_t>(buffer_rx.ant_streams_length_samples));

    // add one additional buffer length
    now_64 += static_cast<int64_t>(buffer_rx.ant_streams_length_samples);

    dectnrp_assert(constants::slots_per_sec % worker_pool_config.rx_ant_streams_length_slots == 0,
                   "not a multiple");

    // determine the number of dummy search to conduct
    const std::size_t n_dummy_searches =
        warmup_sec * constants::slots_per_sec / worker_pool_config.rx_ant_streams_length_slots;

    for (std::size_t i = 0; i < n_dummy_searches; ++i) {
        sync_chunk->wait_for_first_chunk_nto(now_64);

        // run a dummy search
        sync_chunk->search();

        now_64 += static_cast<int64_t>(buffer_rx.ant_streams_length_samples);
    }
}

void worker_sync_t::enqueue_job_nto(const sync_report_t& sync_report) {
    if (baton.is_sync_time_unique(sync_report.fine_peak_time_64)) {
        ++stats.job_packet;

        job_queue.enqueue_nto(job_t(sync_report));
    } else {
        ++stats.job_packet_not_unique;
    }
}

}  // namespace dectnrp::phy
