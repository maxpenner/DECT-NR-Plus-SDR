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

#include "dectnrp/phy/pool/job.hpp"

namespace dectnrp::phy {

class job_slot_t {
    public:
        explicit job_slot_t(const uint32_t id_)
            : id(id_) {};

        const uint32_t id;
        job_t job;

        /// statistics of the slot
        struct stats_t {
                int64_t filled{0};
                int64_t processed{0};
        };

        stats_t stats;
};

}  // namespace dectnrp::phy
