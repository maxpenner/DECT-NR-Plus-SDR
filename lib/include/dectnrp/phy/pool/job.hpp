/*
 * Copyright 2023-present Maxim Penner
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
#include <variant>

#include "dectnrp/application/application_report.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"
#include "dectnrp/phy/rx/sync/regular_report.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"

namespace dectnrp::phy {

class job_t {
    public:
        job_t() = default;

        explicit job_t(const regular_report_t&& regular_report_)
            : content(regular_report_) {}

        explicit job_t(const irregular_report_t&& irregular_report_)
            : content(irregular_report_) {}

        explicit job_t(const sync_report_t& sync_report_)
            : content(sync_report_) {}

        explicit job_t(const application::application_report_t&& application_report_)
            : content(application_report_) {}

        /// regular_report_t is not DefaultConstructible
        std::variant<std::monostate,
                     regular_report_t,
                     irregular_report_t,
                     sync_report_t,
                     application::application_report_t>
            content;

        /// assigned in job_queue_t
        int64_t fifo_cnt{0};
};

}  // namespace dectnrp::phy
