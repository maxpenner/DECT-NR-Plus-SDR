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

#include "dectnrp/common/adt/miscellaneous.hpp"

namespace dectnrp::phy {

class irregular_report_t {
    public:
        irregular_report_t() = default;
        explicit irregular_report_t(const int64_t call_asap_after_this_time_has_passed_64_,
                                    const uint32_t handle_ = 0)
            : call_asap_after_this_time_has_passed_64(call_asap_after_this_time_has_passed_64_),
              handle(handle_) {};

        static constexpr int64_t undefined_late{std::numeric_limits<int64_t>::max()};

        bool has_finite_time() const {
            return call_asap_after_this_time_has_passed_64 < undefined_late;
        }

        irregular_report_t get_same_with_time_increment(const int64_t time_increment_64) const;

        /// when did synchronization recognize this job was due to be put into the job queue?
        int64_t time_of_recognition{common::adt::UNDEFINED_EARLY_64};

        int64_t get_recognition_delay() const {
            return time_of_recognition - call_asap_after_this_time_has_passed_64;
        };

        /**
         * \brief The irregular report will be create as soon as possible after this time has passed
         * for synchronization. The actual call can be delayed by a packet reception.
         */
        int64_t call_asap_after_this_time_has_passed_64{undefined_late};

        /**
         * \brief Handle a firmware can use to identify the irregular called it requested. It is up
         * to the firmware to make sure the handle is unique.
         */
        uint32_t handle{};
};

}  // namespace dectnrp::phy
