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

#include <atomic>
#include <cstdint>
#include <vector>

#include "dectnrp/radio/complex.hpp"
#include "dectnrp/radio/hw_friends.hpp"

// options
// https://en.cppreference.com/w/cpp/atomic/atomic/wait
#define RADIO_BUFFER_RX_NOTIFY_ATOMIC_WAIT 0
// https://en.cppreference.com/w/cpp/thread/condition_variable
#define RADIO_BUFFER_RX_NOTIFY_CV 1
// https://rigtorp.se/spinlock/
#define RADIO_BUFFER_RX_NOTIFY_ATOMIC_BUSYWAIT 2

// choice
#define RADIO_BUFFER_RX_NOTIFY RADIO_BUFFER_RX_NOTIFY_ATOMIC_WAIT

// consequences
#if RADIO_BUFFER_RX_NOTIFY == RADIO_BUFFER_RX_NOTIFY_CV
#include <condition_variable>
#include <mutex>
#endif

/* TCP scope class can be used to debug incoming samples. A corresponding TCP scope flow graph is
 * opened in GNU Radio, which receives the samples through TCP ports and displays them.
 */
// #define RADIO_BUFFER_RX_TCP_SCOPE
#ifdef RADIO_BUFFER_RX_TCP_SCOPE
#include "dectnrp/common/adt/tcp_scope.hpp"
#endif

namespace dectnrp::radio {

class buffer_rx_t {
    public:
        explicit buffer_rx_t(const uint32_t id_,
                             const uint32_t nof_antennas_,
                             const uint32_t ant_streams_length_samples_,
                             const uint32_t samp_rate_,
                             const uint32_t nof_new_samples_max_,
                             const uint32_t rx_prestream_ms_,
                             const uint32_t rx_notification_period_us_);
        ~buffer_rx_t();

        buffer_rx_t() = delete;
        buffer_rx_t(const buffer_rx_t&) = delete;
        buffer_rx_t& operator=(const buffer_rx_t&) = delete;
        buffer_rx_t(buffer_rx_t&&) = delete;
        buffer_rx_t& operator=(buffer_rx_t&&) = delete;

        /// get current time which is the same as the time of the latest sample
        int64_t get_rx_time_passed() const;

        /// get read-only access to antenna samples, should only be called to init local vector
        std::vector<const cf32_t*> get_ant_streams() const;

        /// block until a specific point in time has been reached, nto = no timeout
        int64_t wait_until_nto(const int64_t target_time_64) const;

        const uint32_t id;                          // parent id of hardware
        const uint32_t nof_antennas;                // number of antennas
        const uint32_t ant_streams_length_samples;  // buffer length an external observer can read
        const uint32_t samp_rate;                   // Samples/Second

        HW_FRIENDS;

    private:
        void set_zero();
        void set_zero(const uint32_t idx, const uint32_t length);

        /**
         * \brief Called by hardware to update write pointers, also manages internal time keeping.
         *
         * \param ant_streams_next reference with pointers to be updates
         * \param time_of_first_sample_64 time of first sample written to buffer
         * \param nof_new_samples number of samples written to buffer
         */
        void get_ant_streams_next(std::vector<void*>& ant_streams_next,
                                  const int64_t time_of_first_sample_64,
                                  const uint32_t nof_new_samples);

        /// maximum number of new samples per call of get_ant_streams_next()
        const uint32_t nof_new_samples_max;

        /// our system time is this sample counter
        int64_t time_as_sample_cnt_64;

        /// UHD driver time base is jittering (maybe numerical imprecision), but only +/- 1 sample
        const int64_t acceptable_jitter_range_64;

        /// let hardware first stream some IQ samples before making them available to other threads,
        /// can also be used as a sample counter
        int64_t rx_prestream_64;

        /// one buffer/pointer per antenna stream
        std::vector<cf32_t*> ant_streams;

        /// global system time
        std::atomic<int64_t> rx_time_passed_64;

#if RADIO_BUFFER_RX_NOTIFY == RADIO_BUFFER_RX_NOTIFY_ATOMIC_WAIT || \
    RADIO_BUFFER_RX_NOTIFY == RADIO_BUFFER_RX_NOTIFY_CV
        /// period of notification of waiting threads
        const int64_t notification_period_samples;
        int64_t notification_next;
#endif

#if RADIO_BUFFER_RX_NOTIFY == RADIO_BUFFER_RX_NOTIFY_CV
        mutable std::mutex rx_new_samples_mutex;
        mutable std::condition_variable rx_new_samples_cv;
#endif

#ifdef RADIO_BUFFER_RX_TCP_SCOPE
        std::unique_ptr<common::adt::tcp_scope_t<cf32_t>> tcp_scope;
#endif
};

}  // namespace dectnrp::radio
