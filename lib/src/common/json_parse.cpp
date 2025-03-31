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

#include "dectnrp/common/json_parse.hpp"

#include <fstream>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common::jsonparse {

nlohmann::ordered_json parse(const std::string fullfilepath) {
    std::ifstream f(fullfilepath);

    if (!f.is_open()) {
        dectnrp_assert_failure("Unable to open JSON file {}", fullfilepath);
    }

    return nlohmann::ordered_json::parse(f);
}

bool read_bool(const nlohmann::ordered_json::iterator& it, const std::string field) {
    dectnrp_assert((*it)[field].is_boolean(),
                   "JSON field {} not a boolean or undefined. Check for typos.",
                   field);
    return (*it)[field];
}

std::string read_string(const nlohmann::ordered_json::iterator& it, const std::string field) {
    dectnrp_assert((*it)[field].is_string(),
                   "JSON field {} not a string or undefined. Check for typos.",
                   field);

    const std::string ret = (*it)[field];

    dectnrp_assert(!ret.empty(), "String {} empty", field);

    return ret;
}

int32_t read_int(const nlohmann::ordered_json::iterator& it,
                 const std::string field,
                 const int32_t val_min,
                 const int32_t val_max) {
    dectnrp_assert((*it)[field].is_number_integer(),
                   "JSON field {} not an integer or undefined. Check for typos.",
                   field);

    const int32_t ret = (*it)[field];

    dectnrp_assert(val_min <= ret && ret <= val_max, "Integer {} out of bound", field);

    return ret;
}

std::vector<int32_t> read_int_array(const nlohmann::ordered_json::iterator& it,
                                    const std::string field,
                                    const uint32_t len_min,
                                    const uint32_t len_max,
                                    const uint32_t len_mult) {
    dectnrp_assert((*it)[field].is_array(),
                   "JSON field {} not an array or undefined. Check for typos.",
                   field);

    const uint32_t len = (*it)[field].size();

    dectnrp_assert(len > 0 && len >= len_min && len_max >= len && len % len_mult == 0,
                   "Array length {} incorrect",
                   field);

    // check if first entry is an integer
    dectnrp_assert((*it)[field].at(0).is_number_integer(),
                   "Array {} first entry not an integer or undefined. Check for typos.",
                   field);

    std::vector<int32_t> ret;

    for (uint32_t i = 0; i < len; ++i) {
        ret.push_back((*it)[field].at(i));
    }

    return ret;
}

uint32_t extract_id(const std::string key, const std::string prefix) {
    dectnrp_assert(key.size() > prefix.size(), "key too short");

    return std::stoi(key.substr(prefix.size(), std::string::npos));
}

}  // namespace dectnrp::common::jsonparse
