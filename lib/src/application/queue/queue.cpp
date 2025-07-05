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

#include "dectnrp/application/queue/queue.hpp"

#include <algorithm>
#include <cstring>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::application {

queue_t::queue_t(const queue_size_t queue_size_)
    : queue_size(queue_size_) {
    for (uint32_t i = 0; i < queue_size.N_datagram; ++i) {
        datagram_vec.push_back(new uint8_t[queue_size.N_datagram_max_byte]);
    }

    datagram_level_vec.resize(queue_size.N_datagram, 0);
}

queue_t::~queue_t() {
    for (uint32_t i = 0; i < queue_size.N_datagram; ++i) {
        delete datagram_vec[i];
    }
}

queue_level_t queue_t::get_queue_level_nto(const uint32_t n) const {
    lockv.lock();

    const queue_level_t ret = get_queue_level_under_lock(n);

    lockv.unlock();

    return ret;
}

queue_level_t queue_t::get_queue_level_try(const uint32_t n) const {
    if (!lockv.try_lock()) {
        return queue_level_t();
    }

    const queue_level_t ret = get_queue_level_under_lock(n);

    lockv.unlock();

    return ret;
}

uint32_t queue_t::write_nto(const uint8_t* inp, const uint32_t n) {
    lockv.lock();

    const uint32_t w_ret = write_under_lock(inp, n);

    lockv.unlock();

    return w_ret;
}

uint32_t queue_t::write_try(const uint8_t* inp, const uint32_t n) {
    if (!lockv.try_lock()) {
        return 0;
    }

    const uint32_t w_ret = write_under_lock(inp, n);

    lockv.unlock();

    return w_ret;
}

uint32_t queue_t::read_nto(uint8_t* dst) {
    lockv.lock();

    const uint32_t r_ret = read_under_lock(dst);

    lockv.unlock();

    return r_ret;
}

uint32_t queue_t::read_try(uint8_t* dst) {
    if (!lockv.try_lock()) {
        return 0;
    }

    const uint32_t r_ret = read_under_lock(dst);

    lockv.unlock();

    return r_ret;
}

void queue_t::clear() {
    lockv.lock();
    w_idx = 0;
    r_idx = 0;
    lockv.unlock();
}

queue_level_t queue_t::get_queue_level_under_lock(const uint32_t n) const {
    dectnrp_assert(n <= limits::application_max_queue_level_reported,
                   "number of levels for reporting is limited");

    // make sure returned vector is readable
    const uint32_t n_ = std::min(n, get_used());

    uint32_t r_idx_cpy = r_idx;

    queue_level_t ret;
    ret.N_filled = n_;

    // fill
    for (uint32_t i = 0; i < n_; ++i) {
        ret.levels[i] = datagram_level_vec[r_idx_cpy];
        r_idx_cpy = (r_idx_cpy + 1) % queue_size.N_datagram;
    }

    return ret;
}

uint32_t queue_t::write_under_lock(const uint8_t* inp, const uint32_t n) {
    if (get_free() == 0) {
        return 0;
    }

    dectnrp_assert(n <= queue_size.N_datagram_max_byte, "too large");

    std::memcpy(datagram_vec[w_idx], inp, n);

    datagram_level_vec[w_idx] = n;

    w_idx = (w_idx + 1) % queue_size.N_datagram;

    return n;
}

uint32_t queue_t::read_under_lock(uint8_t* dst) {
    if (get_used() == 0) {
        return 0;
    }

    const uint32_t n = datagram_level_vec[r_idx];

    if (dst != nullptr) {
        std::memcpy(dst, datagram_vec[r_idx], n);
    }

    r_idx = (r_idx + 1) % queue_size.N_datagram;

    return n;
}

uint32_t queue_t::get_free() const {
    if (r_idx > w_idx) {
        // w_idx should never reach r_idx
        return r_idx - w_idx - 1;
    }

    return r_idx + queue_size.N_datagram - w_idx - 1;
}

uint32_t queue_t::get_used() const {
    if (w_idx >= r_idx) {
        return w_idx - r_idx;
    }

    return w_idx + queue_size.N_datagram - r_idx;
}

}  // namespace dectnrp::application
