/*
 * Copyright 2023-2024 Maxim Penner, Alexander Poets
 * Copyright 2025-2025 Maxim Penner
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

namespace dectnrp::section2 {

using absolute_channel_frequency_numbering_t = struct absolute_channel_frequency_numbering_t {
        uint32_t band_number;
        uint32_t n_min;
        uint32_t n_max;
};

using center_frequency_t = struct center_frequency_t {
        absolute_channel_frequency_numbering_t absolute_channel_frequency_numbering;
        uint32_t n;

        int64_t F0_i;
        int64_t channel_spacing_i;
        int64_t FC_i;

        double F0;
        double channel_spacing;
        double FC;
};

using reference_time_accuracy_t = struct reference_time_accuracy_t {
        uint32_t reference_time_category;
        uint32_t accuracy_ppm;
        bool extreme_condition;
};

constexpr uint32_t operating_channel_change_time_requirement_us = 200;

absolute_channel_frequency_numbering_t get_absolute_channel_frequency_numbering(
    const uint32_t band_number);

center_frequency_t get_center_frequency(
    const absolute_channel_frequency_numbering_t absolute_channel_frequency_numbering,
    const uint32_t n);

reference_time_accuracy_t get_reference_time_accuracy(const uint32_t reference_time_category,
                                                      const bool extreme_condition);

bool is_absolute_channel_number_in_range(const uint32_t channel_number);

}  // namespace dectnrp::section2
