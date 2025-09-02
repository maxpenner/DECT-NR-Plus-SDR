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

#define PHY_POOL_JOB_QUEUE_MOODYCAMEL_OR_NAIVE

#ifdef PHY_POOL_JOB_QUEUE_MOODYCAMEL_OR_NAIVE

#include "dectnrp/phy/pool/job_queue_mc.hpp"

namespace dectnrp::phy {
typedef job_queue_mc_t job_queue_t;
}  // namespace dectnrp::phy

#else

#include "dectnrp/phy/pool/job_queue_naive.hpp"

namespace dectnrp::phy {
typedef job_queue_naive_t job_queue_t;
}  // namespace dectnrp::phy

#endif
