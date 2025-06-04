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

#include "dectnrp/common/reporting.hpp"
#include "dectnrp/common/thread/lockable_outer_inner.hpp"
#include "dectnrp/radio/buffer_tx_meta.hpp"
#include "dectnrp/radio/complex.hpp"
#include "dectnrp/radio/hw_friends.hpp"

#define RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
#ifdef RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
#include <condition_variable>
#include <mutex>
#endif

namespace dectnrp::radio {

class buffer_tx_t final : public common::lockable_outer_inner_t, public common::reporting_t {
    public:
        explicit buffer_tx_t(const uint32_t id_,
                             const uint32_t nof_antennas_,
                             const uint32_t ant_streams_length_samples_
#ifdef RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
                             ,
                             std::mutex& tx_new_packet_mutex_,
                             std::condition_variable& tx_new_packet_cv_,
                             uint32_t& tx_new_packet_cnt_
#endif
        );
        ~buffer_tx_t();

        buffer_tx_t() = delete;
        buffer_tx_t(const buffer_tx_t&) = delete;
        buffer_tx_t& operator=(const buffer_tx_t&) = delete;
        buffer_tx_t(buffer_tx_t&&) = delete;
        buffer_tx_t& operator=(buffer_tx_t&&) = delete;

        void get_ant_streams(std::vector<cf32_t*>& ant_streams_, const uint32_t tx_length_samples_);

        void set_transmittable(const struct buffer_tx_meta_t buffer_tx_meta_);

        /**
         * \brief Atomically set current number of samples written. Radio thread will read the value
         * and transmit only as much as PHY has written.
         *
         * \param tx_length_samples_cnt_
         */
        void set_tx_length_samples_cnt(const uint32_t tx_length_samples_cnt_);

        const uint32_t id;
        const uint32_t nof_antennas;
        const uint32_t ant_streams_length_samples;

        friend class buffer_tx_pool_t;

        HW_FRIENDS

    private:
        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

        /**
         * \brief Wait until a target amount of samples has been written. Internally, atomic
         * variable tx_length_samples_cnt is busy-waited on. This function should only be used when
         * the expected waiting time is low and time-critical. Function has no timeout (nto).
         *
         * \param tx_length_samples_target
         */
        void wait_for_samples_busy_nto(const uint32_t tx_length_samples_target) const;

        /// offset relative to first sample
        void get_ant_streams_offset(std::vector<void*>& ant_streams_offset,
                                    const uint32_t offset) const;

        void set_zero(const uint32_t offset, const uint32_t length);
        void set_zero();

        /// called when radio thread has finished reading all samples
        void set_transmitted_or_abort();

        /// put internals into re-lockable state
        void reset();

        /// one buffer/pointer per antenna stream
        std::vector<cf32_t*> ant_streams;

        /// enables back pressure
        std::atomic<uint32_t> tx_length_samples{0};
        std::atomic<uint32_t> tx_length_samples_cnt{0};

        buffer_tx_meta_t buffer_tx_meta{};

#ifdef RADIO_BUFFER_TX_CONDITION_VARIABLE_OR_BUSY_WAITING
        std::mutex& tx_new_packet_mutex;
        std::condition_variable& tx_new_packet_cv;
        uint32_t& tx_new_packet_cnt;
#endif
};

}  // namespace dectnrp::radio
