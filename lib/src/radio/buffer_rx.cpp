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

#include "dectnrp/radio/buffer_rx.hpp"

#include "dectnrp/radio/complex.hpp"

#if RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_BUSY_WAITING
#include "dectnrp/common/thread/watch.hpp"
#endif

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::radio {

buffer_rx_t::buffer_rx_t(const uint32_t id_,
                         const uint32_t nof_antennas_,
                         const uint32_t ant_streams_length_samples_,
                         const uint32_t samp_rate_,
                         const uint32_t nof_new_samples_max_,
                         const uint32_t rx_prestream_ms_,
#if RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_BUSY_WAITING
                         [[maybe_unused]] const uint32_t rx_notification_period_us_)
#else
                         const uint32_t rx_notification_period_us_)
#endif
    : id(id_),
      nof_antennas(nof_antennas_),
      ant_streams_length_samples(ant_streams_length_samples_),
      samp_rate(samp_rate_),
      nof_new_samples_max(nof_new_samples_max_),
      time_as_sample_cnt_64(0),
      acceptable_jitter_range_64(1),
      rx_prestream_64(static_cast<int64_t>(samp_rate) * static_cast<int64_t>(rx_prestream_ms_) /
                      int64_t{1000})
#if RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_ATOMIC_WAIT || \
    RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_CONDITION_VARIABLE
      ,
      notification_period_samples(static_cast<int64_t>(samp_rate) *
                                  static_cast<int64_t>(rx_notification_period_us_) /
                                  int64_t{1000000}),
      notification_next(0)
#endif
{
    dectnrp_assert(nof_new_samples_max * 8 <= ant_streams_length_samples,
                   "Buffer should be at least 8 times larger.");

    // internally, buffers are kept slightly longer so that we are always able to write
    // nof_new_samples_max samples
    for (uint32_t i = 0; i < nof_antennas; ++i) {
        ant_streams.push_back(cf32_malloc(ant_streams_length_samples + nof_new_samples_max));
    }

    set_zero();

    // Assume we start the system at time t=0. When we have received exactly one sample, this means
    // one sample time has passed and we are now at time t=1.
    rx_time_passed_64.store(0, std::memory_order_release);

#ifdef RADIO_BUFFER_RX_TCP_SCOPE
    tcp_scope = std::make_unique<common::adt::tcp_scope_t<cf32_t>>(2200, nof_antennas);
#endif
}

buffer_rx_t::~buffer_rx_t() {
    for (auto& elem : ant_streams) {
        free(elem);
    }
}

int64_t buffer_rx_t::get_rx_time_passed() const {
    return rx_time_passed_64.load(std::memory_order_acquire);
};

std::vector<const cf32_t*> buffer_rx_t::get_ant_streams() const {
    // https://stackoverflow.com/questions/33126511/const-method-in-a-class-returning-vector-of-pointers
    return std::vector<const cf32_t*>(ant_streams.begin(), ant_streams.end());
};

int64_t buffer_rx_t::wait_until_nto(const int64_t target_time_64) const {
    // has the calling thread an old timestamp? if so, immediately return latest time
    int64_t now_64 = rx_time_passed_64.load(std::memory_order_acquire);
    if (target_time_64 < now_64) {
        return now_64;
    }

    // target time not reached yet, so we have to wait
    do {
#if RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_ATOMIC_WAIT
        rx_time_passed_64.wait(now_64, std::memory_order_acquire);
#elif RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_CONDITION_VARIABLE
        std::unique_lock<std::mutex> lk(rx_new_samples_mutex);
        rx_new_samples_cv.wait(lk);
#elif RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_BUSY_WAITING
        // limit calls to atomic
        common::watch_t::busywait_us();
#endif
        now_64 = rx_time_passed_64.load(std::memory_order_acquire);
    } while (now_64 < target_time_64);

    return now_64;
}

void buffer_rx_t::set_zero() { set_zero(0, ant_streams_length_samples + nof_new_samples_max); }

void buffer_rx_t::set_zero(const uint32_t idx, const uint32_t length) {
    dectnrp_assert(idx + length <= ant_streams_length_samples + nof_new_samples_max,
                   "Zeroing more than maximum");

    for (uint32_t i = 0; i < ant_streams.size(); ++i) {
        cf32_zero(&ant_streams[i][idx], length);
    }
}

void buffer_rx_t::get_ant_streams_next(std::vector<void*>& ant_streams_next,
                                       const int64_t time_of_first_sample_64,
                                       const uint32_t nof_new_samples) {
    dectnrp_assert(nof_new_samples <= nof_new_samples_max,
                   "Number of newly written samples larger maximum.");

    // measure the jitter between the internal sample counter and the external absolute time
    // provided by the hardware
    int64_t time_error = time_as_sample_cnt_64 - time_of_first_sample_64;

    // we accept only a small amount of jitter
    if (time_error < -acceptable_jitter_range_64 || acceptable_jitter_range_64 < time_error) {
        dectnrp_assert(time_as_sample_cnt_64 >= 0, "time error");

        /* We are outside the jitter range, so now it is likely that the hardware dropped samples,
         * but we don't necessarily know how many. As a countermeasure, we simply set the sample
         * counter to the latest absolute time given by the hardware. With the next call of this
         * function, we should be locked backed into the jitter range.
         */
        time_as_sample_cnt_64 = time_of_first_sample_64;
    }

    // convert global time to local write index hw has used
    uint32_t index = static_cast<uint32_t>(time_as_sample_cnt_64 %
                                           static_cast<int64_t>(ant_streams_length_samples));

    // check if the last write process has written beyond ant_streams_length_samples
    if ((index + nof_new_samples) > ant_streams_length_samples) {
        const uint32_t nof_samples_to_wrap = index + nof_new_samples - ant_streams_length_samples;

        // wrap each antenna stream
        for (uint32_t i = 0; i < ant_streams.size(); ++i) {
            cf32_copy(
                ant_streams[i], &ant_streams[i][ant_streams_length_samples], nof_samples_to_wrap);
        }
    }

#ifdef RADIO_BUFFER_RX_TCP_SCOPE
    std::vector<cf32_t*> ant_streams_for_scope;
    for (uint32_t i = 0; i < nof_antennas; ++i) {
        ant_streams_for_scope.push_back(&ant_streams[i][index]);
    }
    tcp_scope->send_to_scope(ant_streams_for_scope, nof_new_samples);
#endif

    // increase internal time
    time_as_sample_cnt_64 += static_cast<int64_t>(nof_new_samples);

    // convert updated global time to local write index hw has used
    index = static_cast<uint32_t>(time_as_sample_cnt_64 %
                                  static_cast<int64_t>(ant_streams_length_samples));

    // get and convert address of next complex sample index to write to
    for (uint32_t i = 0; i < nof_antennas; ++i) {
        ant_streams_next[i] = reinterpret_cast<void*>(&ant_streams[i][index]);
    }

    /* ##################################################
     * Up until this point we have made only internal changes, i.e. written samples, checked the
     * jitter, wrapped, updated pointers etc.
     *
     * Now begins the public interface, i.e. we make the internal changes available to waiting
     * threads. We do so by updating the global atomic rx_time_passed_64 every external thread
     * reads, and then by notifying said threads.
     */

    // let hardware stream a bit before to get everything up and running
    rx_prestream_64 -= nof_new_samples;
    if (0 <= rx_prestream_64) {
        return;
    }

    // save latest time in atomic
    rx_time_passed_64.store(time_as_sample_cnt_64, std::memory_order_release);

#if RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_ATOMIC_WAIT
    if (time_as_sample_cnt_64 >= notification_next) {
        // wake up any threads waiting for new samples
        rx_time_passed_64.notify_all();
        notification_next = time_as_sample_cnt_64 + notification_period_samples;
    }
#elif RADIO_BUFFER_RX_NOTIFICATION_MECHANISM == RADIO_BUFFER_RX_CONDITION_VARIABLE
    if (time_as_sample_cnt_64 >= notification_next) {
        /* This thread is not allowed to get blocked under any circumstances. Therefore, only try to
         * get a lock. If not possible, we will publish at a later point in time. Other threads
         * should not hold onto this lock for too long.
         */
        if (rx_new_samples_mutex.try_lock()) {
            // wake up any threads waiting for new samples
            rx_new_samples_cv.notify_all();
            notification_next = time_as_sample_cnt_64 + notification_period_samples;
            rx_new_samples_mutex.unlock();
        }
    }
#endif
}

}  // namespace dectnrp::radio
