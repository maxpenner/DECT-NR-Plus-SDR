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

/**
 * \brief Exporting JSON data during runtime can have a severe impact on performance, throughput,
 * latency etc. But even if we don't export JSON files, simply having the functions and not calling
 * them because of an if-else checked at runtime may have a negative performance impact. This
 * directive allows deactivating relevant code sections that are used for JSON file export.
 */
#define PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
