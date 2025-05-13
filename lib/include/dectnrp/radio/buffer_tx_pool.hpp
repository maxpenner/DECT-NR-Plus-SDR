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
#include <memory>

#include "dectnrp/radio/buffer_tx.hpp"
#include "dectnrp/radio/hw_friends.hpp"

namespace dectnrp::radio {

class buffer_tx_pool_t {
    public:
        explicit buffer_tx_pool_t(const uint32_t id_,
                                  const uint32_t nof_antennas_,
                                  const uint32_t nof_buffer_tx_,
                                  const uint32_t ant_streams_length_samples_);
        ~buffer_tx_pool_t() = default;

        buffer_tx_pool_t() = delete;
        buffer_tx_pool_t(const buffer_tx_pool_t&) = delete;
        buffer_tx_pool_t& operator=(const buffer_tx_pool_t&) = delete;
        buffer_tx_pool_t(buffer_tx_pool_t&&) = delete;
        buffer_tx_pool_t& operator=(buffer_tx_pool_t&&) = delete;

        /**
         * \brief Multiple Producers (Threads on PHY)
         *
         * Called by PHY to get a fillable buffer_tx. Actual filling and transmitting is then done
         * with the help of member functions of buffer_tx. If no buffer is available, pointer will
         * be nullptr. System should be designed is such a way that an unavailability of buffer_tx
         * never occurs, in any case, the caller decides on how to react to that.
         *
         * \return pointer to buffer_tx_t, or nullptr
         */
        buffer_tx_t* get_buffer_tx_to_fill();

        const uint32_t id;
        const uint32_t nof_antennas;
        const uint32_t nof_buffer_tx;
        const uint32_t ant_streams_length_samples;

        HW_FRIENDS

    private:
        std::vector<std::unique_ptr<buffer_tx_t>> buffer_tx_vec;

        /**
         * \brief Buffers can be written by independent producers and therefore are unordered in
         * terms of tx_order_id and transmission time. This function can be called to check if a
         * packet with the specified tx_order_id exists and, if so, what its local index is. Return
         * value can be -1 if the buffer does not exist yet.
         *
         * \param tx_order_id_target
         * \return
         */
        int32_t get_specific_tx_order_id_if_available(const int64_t tx_order_id_target) const;

        /**
         * \brief Waits for packet with specific tx order. Once it is available, return the index of
         * the buffer_tx in buffer_tx_vec. This function has a timeout (to).
         *
         * \param tx_order_id_target
         * \param timeout_ms
         * \return
         */
        int32_t wait_for_specific_tx_order_id_to(const int64_t tx_order_id_target,
                                                 const uint32_t timeout_ms) const;

        /**
         * \brief Same as above, but keeps spinning until required packet was found. Function is
         * used on layer radio for time-critical consecutive transmissions. This function has a
         * timeout (to).
         *
         * \param tx_order_id_target
         * \param timeout_us
         * \return
         */
        int32_t wait_for_specific_tx_order_id_busy_to(const int64_t tx_order_id_target,
                                                      const uint32_t timeout_us) const;

        /// vector of pointers to allocated buffer_tx, ownership is not shared
        std::vector<buffer_tx_t*> get_buffer_tx_vec() const;

#ifdef PHY_BUFFER_TX_NOTIFIES_CONDITION_VARIABLE_OF_BUFFER_TX_POOL
        mutable std::mutex tx_new_packet_mutex;
        mutable std::condition_variable tx_new_packet_cv;
        uint32_t tx_new_packet_cnt;
#endif
};

}  // namespace dectnrp::radio
