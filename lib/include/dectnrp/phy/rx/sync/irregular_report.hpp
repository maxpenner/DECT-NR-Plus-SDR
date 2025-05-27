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
#include <limits>

namespace dectnrp::phy {

class irregular_report_t {
    public:
        irregular_report_t() = default;
        explicit irregular_report_t(const int64_t call_asap_after_this_time_has_passed_64_,
                                    const uint32_t handle_)
            : call_asap_after_this_time_has_passed_64(call_asap_after_this_time_has_passed_64_),
              handle(handle_){};

        static constexpr int64_t undefined_late{std::numeric_limits<int64_t>::max()};

        /**
         * \brief The irregular report will be create as soon as possible after this time has passed
         * for synchronization. The actual call can be delayed by a packet reception.
         */
        int64_t call_asap_after_this_time_has_passed_64{std::numeric_limits<int64_t>::max()};

        /**
         * \brief Handle a firmware can use to identify the irregular called it requested. It is up
         * to the firmware to make sure the handle is unique.
         */
        uint32_t handle{};

        bool has_finite_time() const {
            return call_asap_after_this_time_has_passed_64 < undefined_late;
        }
};

}  // namespace dectnrp::phy
