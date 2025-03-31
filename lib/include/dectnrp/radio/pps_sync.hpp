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

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace dectnrp::radio {

class hw_t;

class pps_sync_t {
    public:
        void expect_one_more_hw() { ++nof_hw; };

        void sync_procedure(hw_t* hw);

    private:
        static constexpr uint32_t CV_WAIT_TIMEOUT_MS{100};
        static constexpr uint32_t CV_WAIT_WATCHDOG_MS{10000};

        uint32_t nof_hw{0};
        uint32_t nof_hw_cnt{0};
        std::mutex mtx;
        std::condition_variable cv;
};

}  // namespace dectnrp::radio
