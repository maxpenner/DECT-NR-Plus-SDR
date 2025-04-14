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

#define PLL_PARAM_DIST_MIN_ACCEPT_MS 1000

#define PLL_PARAM_DIST_MIN_MS 5000
#define PLL_PARAM_DIST_MIN_TO_MAX_IN_BEACON_PERIODS 8

#define PLL_PARAM_DIST_MAX_DEVIATION_SAFETY_FACTOR 8

#define PLL_PARAM_EMA_ALPHA 0.9

// #define PLL_PARAM_UNLUCKY_WARP_FACTOR_FILTER

#define PLL_PARAM_PPM_OUT_OF_SYNC 150.0
