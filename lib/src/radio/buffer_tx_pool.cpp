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

#include "dectnrp/radio/buffer_tx_pool.hpp"

#include <vector>

#include "dectnrp/common/thread/watch.hpp"

namespace dectnrp::radio {

buffer_tx_pool_t::buffer_tx_pool_t(const uint32_t id_,
                                   const uint32_t nof_antennas_,
                                   const uint32_t nof_buffer_tx_,
                                   const uint32_t ant_streams_length_samples_)
    : id(id_),
      nof_antennas(nof_antennas_),
      nof_buffer_tx(nof_buffer_tx_),
      ant_streams_length_samples(ant_streams_length_samples_) {
    for (uint32_t buffer_id = 0; buffer_id < nof_buffer_tx; ++buffer_id) {
        buffer_tx_vec.push_back(std::make_unique<buffer_tx_t>(buffer_id,
                                                              nof_antennas,
                                                              ant_streams_length_samples
#ifdef PHY_BUFFER_TX_NOTIFIES_CONDITION_VARIABLE_OF_BUFFER_TX_POOL
                                                              ,
                                                              tx_new_packet_mutex,
                                                              tx_new_packet_cv,
                                                              tx_new_packet_cnt
#endif
                                                              ));
    }

#ifdef PHY_BUFFER_TX_NOTIFIES_CONDITION_VARIABLE_OF_BUFFER_TX_POOL
    tx_new_packet_cnt = 0;
#endif
};

buffer_tx_t* buffer_tx_pool_t::get_buffer_tx_to_fill() {
    // go over each buffer once ...
    for (uint32_t i = 0; i < nof_buffer_tx; ++i) {
        // ... and check if it is lockable
        if (buffer_tx_vec[i]->try_lock_outer()) {
            // if so, return a pointer to that exact buffer
            return buffer_tx_vec[i].get();
        }
    }

    // ideally, this point is never reached as there always is a free buffer
    return nullptr;
}

int32_t buffer_tx_pool_t::get_specific_tx_order_id_if_available(
    const int64_t tx_order_id_target) const {
    // go over each buffer ...
    for (uint32_t i = 0; i < nof_buffer_tx; ++i) {
        // ... and check whether it is transmittable ...
        if (buffer_tx_vec[i]->is_inner_locked()) {
            // ... and, if so, whether it has the tx order id we are looking for
            if (buffer_tx_vec[i]->buffer_tx_meta.tx_order_id == tx_order_id_target) {
                // return the index of the buffer
                return i;
            }
        }
    }

    // buffer not found
    return -1;
}

int32_t buffer_tx_pool_t::wait_for_specific_tx_order_id_to(const int64_t tx_order_id_target,
                                                           const uint32_t timeout_ms) const {
#ifdef PHY_BUFFER_TX_NOTIFIES_CONDITION_VARIABLE_OF_BUFFER_TX_POOL
    // initial search
    int32_t ret = get_specific_tx_order_id_if_available(tx_order_id_target);

    if (ret >= 0) {
        return ret;
    }

    {
        std::unique_lock<std::mutex> lk(tx_new_packet_mutex);

        /* At this point, we locked the mutex and no other thread can increment tx_new_packet_cnt.
         * We have not yet checked any of the TX buffers, so we set the reference value to zero.
         */
        uint32_t tx_new_packet_cnt_ref = 0;

        /* The following loop can only be left if either ...
         * 1) The right buffer_tx was found, i.e. ret >= 0.
         * 2) The condition variable timed out.
         */
        while (ret < 0) {
            while (tx_new_packet_cnt == tx_new_packet_cnt_ref) {
                // implicitly unlock the mutex and wait for a condition variable
                auto res = tx_new_packet_cv.wait_for(lk, std::chrono::milliseconds(timeout_ms));

                // cv timed out, leave function without job and by that remove the lock on the mutex
                if (res == std::cv_status::timeout) {
                    return -1;
                }
            }

            // we have the lock on the mutex, check if correct buffer_tx is available
            ret = get_specific_tx_order_id_if_available(tx_order_id_target);

            // save amount of oldest packets already checked
            tx_new_packet_cnt_ref = tx_new_packet_cnt;
        }
    }

    return ret;

#else

    return wait_for_specific_tx_order_id_busy_to(tx_order_id_target, timeout_ms * 1000);

#endif
}

int32_t buffer_tx_pool_t::wait_for_specific_tx_order_id_busy_to(const int64_t tx_order_id_target,
                                                                const uint32_t timeout_us) const {
    // initial search
    int32_t ret = get_specific_tx_order_id_if_available(tx_order_id_target);

    if (ret >= 0) {
        return ret;
    }

    common::watch_t watch;

    while (ret < 0) {
        // limit calls to atomic
        common::watch_t::busywait_us();

        ret = get_specific_tx_order_id_if_available(tx_order_id_target);

        // timeout after some microseconds
        if (watch.is_elapsed<common::micro>(timeout_us)) {
            break;
        }
    }

    return ret;
}

std::vector<buffer_tx_t*> buffer_tx_pool_t::get_buffer_tx_vec() const {
    std::vector<buffer_tx_t*> ret;

    for (auto& elem : buffer_tx_vec) {
        ret.push_back(elem.get());
    }

    return ret;
}

}  // namespace dectnrp::radio
