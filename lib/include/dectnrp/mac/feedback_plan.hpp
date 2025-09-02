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
#include <vector>

namespace dectnrp::mac {

class feedback_plan_t {
    public:
        feedback_plan_t() = default;
        feedback_plan_t(const std::vector<uint32_t>&& feedback_format_vec);

        [[nodiscard]] uint32_t get_current_feedback_format();
        void set_next_feedback_format();

    private:
        std::vector<uint32_t> feedback_format_vec;
        std::size_t idx{};
};

}  // namespace dectnrp::mac
