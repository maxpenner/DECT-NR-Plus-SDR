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
#include <string>
#include <vector>

#include "header_only/nlohmann/json.hpp"

namespace dectnrp::common::jsonparse {

nlohmann::ordered_json parse(const std::string fullfilepath);

bool read_bool(const nlohmann::ordered_json::iterator& it, const std::string field);

std::string read_string(const nlohmann::ordered_json::iterator& it, const std::string field);

int32_t read_int(const nlohmann::ordered_json::iterator& it,
                 const std::string field,
                 const int32_t val_min,
                 const int32_t val_max);

std::vector<int32_t> read_int_array(const nlohmann::ordered_json::iterator& it,
                                    const std::string field,
                                    const uint32_t len_min,
                                    const uint32_t len_max,
                                    const uint32_t len_mult);

uint32_t extract_id(const std::string key, const std::string prefix);

}  // namespace dectnrp::common::jsonparse
