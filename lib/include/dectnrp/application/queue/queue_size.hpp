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

#include "dectnrp/limits.hpp"

namespace dectnrp::application {

struct queue_size_t {
        uint32_t N_item{};
        uint32_t N_item_max_byte{};

        bool is_valid() const {
            if (N_item == 0 || limits::app_max_queue_item < N_item) {
                return false;
            }

            if (N_item_max_byte == 0 || limits::app_max_queue_item_size < N_item_max_byte) {
                return false;
            }

            return true;
        }
};

}  // namespace dectnrp::application
