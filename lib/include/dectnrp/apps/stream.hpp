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

namespace dectnrp::apps {

class stream_t {
    public:
        stream_t() = default;
        stream_t(const std::size_t id_,
                 const double period_sec_,
                 const double offset_sec_,
                 const std::size_t repetitions_);

        static constexpr int64_t mega{1000000};

        int64_t set_start_full_sec(const int64_t full_sec_64);
        int64_t get_next_us();
        void generate_payload(uint8_t* payload, const std::size_t payload_size_byte);

        struct stats_t {
                uint64_t tx{};
        };

        stats_t get_stats() const { return stats; };

    private:
        const std::size_t id{};
        const double period_sec{};
        const double offset_sec{};
        const std::size_t repetitions{};

        /// count repetitions in current second
        std::size_t repetitions_cnt{};

        /// count full seconds
        int64_t full_sec_cnt_64{};

        stats_t stats;
};

}  // namespace dectnrp::apps
