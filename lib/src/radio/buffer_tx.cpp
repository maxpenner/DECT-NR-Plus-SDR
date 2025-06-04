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

#include "dectnrp/radio/buffer_tx.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/radio/complex.hpp"

namespace dectnrp::radio {

buffer_tx_t::buffer_tx_t(const uint32_t id_,
                         const uint32_t nof_antennas_,
                         const uint32_t ant_streams_length_samples_
#ifdef RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
                         ,
                         std::mutex& tx_new_packet_mutex_,
                         std::condition_variable& tx_new_packet_cv_,
                         uint32_t& tx_new_packet_cnt_
#endif
                         )
    : id(id_),
      nof_antennas(nof_antennas_),
      ant_streams_length_samples(ant_streams_length_samples_)
#ifdef RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
      ,
      tx_new_packet_mutex(tx_new_packet_mutex_),
      tx_new_packet_cv(tx_new_packet_cv_),
      tx_new_packet_cnt(tx_new_packet_cnt_)
#endif

{
    for (uint32_t i = 0; i < nof_antennas; ++i) {
        ant_streams.push_back(cf32_malloc(ant_streams_length_samples));
    }

    dectnrp_assert(!is_outer_locked(), "incorrect lock state");
    dectnrp_assert(!is_inner_locked(), "incorrect lock state");

    dectnrp_assert(tx_length_samples == 0, "incorrect lock state");
    dectnrp_assert(tx_length_samples_cnt == 0, "incorrect lock state");

    dectnrp_assert(buffer_tx_meta.tx_order_id = -1, "incorrect lock state");
    dectnrp_assert(buffer_tx_meta.tx_time_64 = -1, "incorrect lock state");
}

buffer_tx_t::~buffer_tx_t() {
    for (auto& elem : ant_streams) {
        free(elem);
    }
}

void buffer_tx_t::get_ant_streams(std::vector<cf32_t*>& ant_streams_,
                                  const uint32_t tx_length_samples_) {
    dectnrp_assert(is_outer_locked_inner_unlocked(), "incorrect lock state");
    dectnrp_assert(ant_streams_.size() == nof_antennas, "incorrect ant_streams size");

    for (uint32_t i = 0; i < nof_antennas; ++i) {
        ant_streams_[i] = ant_streams[i];
    }

    dectnrp_assert(tx_length_samples_ <= ant_streams_length_samples,
                   "TX length longer than TX buffer");

    tx_length_samples.store(tx_length_samples_, std::memory_order_release);
}

void buffer_tx_t::set_transmittable(const struct buffer_tx_meta_t buffer_tx_meta_) {
    dectnrp_assert(is_outer_locked_inner_unlocked(), "incorrect lock state");

    buffer_tx_meta = buffer_tx_meta_;

    lock_inner();

#ifdef RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
    // make sure TX thread is notified of this new packet and woken up at least once
    {
        std::unique_lock<std::mutex> lk(tx_new_packet_mutex);
        ++tx_new_packet_cnt;
    }

    tx_new_packet_cv.notify_all();
#endif
}

void buffer_tx_t::set_tx_length_samples_cnt(const uint32_t tx_length_samples_cnt_) {
    dectnrp_assert(tx_length_samples_cnt_ <= ant_streams_length_samples,
                   "TX length counter longer than TX buffer");

    tx_length_samples_cnt.store(tx_length_samples_cnt_, std::memory_order_release);
}

std::vector<std::string> buffer_tx_t::report_start() const { return std::vector<std::string>(); }

std::vector<std::string> buffer_tx_t::report_stop() const {
    return std::vector<std::string>{
        std::string("TX Buffer " + std::to_string(id) + " " + get_stats_as_string())};
}

void buffer_tx_t::wait_for_samples_busy_nto(const uint32_t tx_length_samples_target) const {
    dectnrp_assert(tx_length_samples_target <= tx_length_samples.load(std::memory_order_acquire),
                   "Announced less samples than waiting for");

    while (tx_length_samples_cnt.load(std::memory_order_acquire) < tx_length_samples_target) {
        /* We busy-wait in this while loop as it is time-critical. Most of the time we're not gonna
         * enter this while loop because the respective PHY thread has already written enough
         * samples, i.e. tx_length_samples_cnt is large enough. This is especially true if the PHY
         * threads are fast and flag the TX buffer as transmittable only after writing a fair amount
         * of samples. As a rule of thumb: Each PHY thread has to try to stay ahead of the radio
         * thread.
         */
        common::watch_t::busywait_us();
    }
}

void buffer_tx_t::get_ant_streams_offset(std::vector<void*>& ant_streams_offset,
                                         const uint32_t offset) const {
    dectnrp_assert(is_outer_locked_inner_locked(), "incorrect lock state");

    // get and convert address of next complex sample index to write to
    for (uint32_t i = 0; i < nof_antennas; ++i) {
        ant_streams_offset[i] = reinterpret_cast<void*>(&ant_streams[i][offset]);
    }
}

void buffer_tx_t::set_zero(const uint32_t offset, const uint32_t length) {
    dectnrp_assert(is_outer_locked_inner_locked(), "incorrect lock state");
    dectnrp_assert(offset + length <= ant_streams_length_samples,
                   "Zeroing beyond length of TX buffer");

    for (uint32_t i = 0; i < nof_antennas; ++i) {
        cf32_zero(&ant_streams[i][offset], length);
    }
}

void buffer_tx_t::set_zero() { set_zero(0, ant_streams_length_samples); }

void buffer_tx_t::set_transmitted_or_abort() {
    dectnrp_assert(is_outer_locked_inner_locked(), "incorrect lock state");

    reset();
    unlock_inner();
    unlock_outer();

#ifdef RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
    {
        std::unique_lock<std::mutex> lk(tx_new_packet_mutex);
        tx_new_packet_cnt--;
    }
#endif
}

void buffer_tx_t::reset() {
    dectnrp_assert(is_outer_locked_inner_locked(), "incorrect lock state");

    tx_length_samples.store(0, std::memory_order_release);
    tx_length_samples_cnt.store(0, std::memory_order_release);

    buffer_tx_meta.tx_order_id = -1;
    buffer_tx_meta.tx_time_64 = -1;
}

}  // namespace dectnrp::radio
