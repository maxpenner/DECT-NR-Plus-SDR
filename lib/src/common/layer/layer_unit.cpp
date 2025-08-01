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

#include "dectnrp/common/layer/layer_unit.hpp"

#include "dectnrp/common/prog/log.hpp"

namespace dectnrp::common {

void layer_unit_t::log_line([[maybe_unused]] const std::string line) const {
    dectnrp_log_inf("{}", std::string(identifier + " | " + line));
}

void layer_unit_t::log_lines(const std::vector<std::string> lines) const {
    for (const auto& line : lines) {
        log_line(line);
    }
}

}  // namespace dectnrp::common
