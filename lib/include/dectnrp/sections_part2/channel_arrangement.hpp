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

struct absolute_channel_frequency_numbering_t {
        uint32_t band_number;
        uint32_t n_min;
        uint32_t n_max;

        /**
         * \brief"The actual operating channel raster (channel grid) and RF bandwidth restriction in
         * each band may vary depending on relevant regulatory requirements."
         */
        uint32_t n_spacing;
};

struct center_frequency_t {
        absolute_channel_frequency_numbering_t acfn;
        uint32_t n;

        int64_t F0_i;
        int64_t channel_spacing_i;
        int64_t FC_i;

        double F0;
        double channel_spacing;
        double FC;
};

constexpr uint32_t operating_channel_change_time_requirement_us{200};

absolute_channel_frequency_numbering_t get_absolute_channel_frequency_numbering(
    const uint32_t band_number);

center_frequency_t get_center_frequency(const absolute_channel_frequency_numbering_t acfn,
                                        const uint32_t n);

center_frequency_t get_center_frequency(const uint32_t band_number, const uint32_t n);

bool is_absolute_channel_number_in_range(const uint32_t channel_number);

}  // namespace dectnrp::section2
