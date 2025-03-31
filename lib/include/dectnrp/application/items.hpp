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

#include <cstdint>
#include <vector>

#define APPLICATION_ITEMS_MUTEX_OR_SPINLOCK
#ifdef APPLICATION_ITEMS_MUTEX_OR_SPINLOCK
#include <mutex>
#else
#include "dectnrp/common/thread/spinlock.hpp"
#endif

#include "dectnrp/application/items_level_report.hpp"

namespace dectnrp::application {

class items_t {
    public:
        /**
         * \brief The SDR accepts chunks of data as input, for instance UDP packet payloads or
         * entire IP packets when using a VNIC. Each of these chunks is called an item and this
         * class contains multiple of these items. An item can be written to this class or read from
         * it. Furthermore, classes can request the current status of each instance of items_t.
         *
         * \param N_item_ number of items than can be queued
         * \param N_item_byte_ maximum number of bytes in each item, less can be written and read
         */
        explicit items_t(const uint32_t N_item_, const uint32_t N_item_byte_);
        ~items_t();

        items_t() = delete;
        items_t(const items_t&) = delete;
        items_t& operator=(const items_t&) = delete;
        items_t(items_t&&) = delete;
        items_t& operator=(items_t&&) = delete;

        /**
         * \brief Get current level of the last n items. The level is the number of bytes written to
         * an item. If less than n items are filled, the return vector is shorter than n. Function
         * is thread-safe and waits for the lock indefinitely, i.e. with no timeout (nto).
         *
         * \param n number of oldest items to consider
         * \return
         */
        items_level_report_t get_items_level_report_nto(const uint32_t n) const;
        items_level_report_t get_items_level_report_try(const uint32_t n) const;

        /**
         * \brief Write a new item to the queue. If all internal items are used, items is not
         * written. Function is thread-safe and waits for the lock indefinitely, i.e. with no
         * timeout (nto).
         *
         * \param inp source
         * \param n number of bytes, i.e. level
         * \return either 0 is all internal items are used, or n
         */
        [[nodiscard]] uint32_t write_nto(const uint8_t* inp, const uint32_t n);
        [[nodiscard]] uint32_t write_try(const uint8_t* inp, const uint32_t n);

        /**
         * \brief Copy binary data of oldest item to dst. With no timeout means that we wait for the
         * lock indefinitely. Function is thread-safe and waits for the lock indefinitely, i.e. with
         * no timeout (nto).
         *
         * \param dst if set to nullptr oldest item is invalidated, i.e. read without copying
         * \return number of bytes read
         */
        [[nodiscard]] uint32_t read_nto(uint8_t* dst);
        [[nodiscard]] uint32_t read_try(uint8_t* dst);

        const uint32_t N_item;
        const uint32_t N_item_byte;

    private:
        uint32_t w_idx{0};
        uint32_t r_idx{0};

#ifdef APPLICATION_ITEMS_MUTEX_OR_SPINLOCK
        mutable std::mutex lockv;
#else
        mutable common::spinlock_t lockv;
#endif

        std::vector<uint8_t*> items_vec;
        std::vector<uint32_t> items_level_vec;

        items_level_report_t get_items_level_report_under_lock(const uint32_t n) const;
        uint32_t write_under_lock(const uint8_t* inp, const uint32_t n);
        uint32_t read_under_lock(uint8_t* dst);

        uint32_t get_free() const;
        uint32_t get_used() const;
};

}  // namespace dectnrp::application
