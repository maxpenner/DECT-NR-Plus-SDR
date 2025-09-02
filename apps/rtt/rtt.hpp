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

/// priority and core, 0 implies highest priority possible
#define RTT_RUN_WITH_THREAD_PRIORITY_OFFSET 0
#define RTT_RUN_ON_CORE 9

/// print summary after running this fixed number of measurements
#define RTT_MEASUREMENTS_PER_PRINT 100000

/// gap between two measurements, 0 means ASAP
#define RTT_MEASUREMENT_TO_MEASUREMENT_SLEEP_US 0

/// how long before we assume a packet was not acknowledged?
#define RTT_UDP_TIMEOUT_BEFORE_ASSUMING_ERROR_US 100000
