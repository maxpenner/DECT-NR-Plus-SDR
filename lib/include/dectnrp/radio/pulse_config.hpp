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

#include "dectnrp/common/adt/miscellaneous.hpp"

namespace dectnrp::radio {

struct pulse_config_t {
        pulse_config_t(const int64_t rising_edge_64_, const int64_t falling_edge_64_)
            : rising_edge_64(rising_edge_64_),
              falling_edge_64(falling_edge_64_){};

        int64_t rising_edge_64{common::adt::UNDEFINED_EARLY_64};
        int64_t falling_edge_64{common::adt::UNDEFINED_EARLY_64};
};

}  // namespace dectnrp::radio
