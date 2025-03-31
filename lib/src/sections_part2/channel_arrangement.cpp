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

#include "dectnrp/sections_part2/channel_arrangement.hpp"

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section2 {

absolute_channel_frequency_numbering_t get_absolute_channel_frequency_numbering(
    const uint32_t band_number) {
    dectnrp_assert(1 <= band_number || band_number <= 19, "Band number must be between 1 and 19");

    absolute_channel_frequency_numbering_t absolute_channel_frequency_numbering{.band_number =
                                                                                    band_number};

    switch (band_number) {
        case 1:
            absolute_channel_frequency_numbering.n_min = 1657;
            absolute_channel_frequency_numbering.n_max = 1677;
            break;

        case 2:
            absolute_channel_frequency_numbering.n_min = 1680;
            absolute_channel_frequency_numbering.n_max = 1700;
            break;

        case 3:
            absolute_channel_frequency_numbering.n_min = 2258;
            absolute_channel_frequency_numbering.n_max = 2352;
            break;

        case 4:
            absolute_channel_frequency_numbering.n_min = 524;
            absolute_channel_frequency_numbering.n_max = 552;
            break;

        case 5:
            absolute_channel_frequency_numbering.n_min = 1;
            absolute_channel_frequency_numbering.n_max = 22;
            break;

        case 6:
            absolute_channel_frequency_numbering.n_min = 288;
            absolute_channel_frequency_numbering.n_max = 411;
            break;

        case 7:
            absolute_channel_frequency_numbering.n_min = 309;
            absolute_channel_frequency_numbering.n_max = 321;
            break;

        case 8:
            absolute_channel_frequency_numbering.n_min = 1137;
            absolute_channel_frequency_numbering.n_max = 1234;
            break;

        case 9:
            absolute_channel_frequency_numbering.n_min = 1691;
            absolute_channel_frequency_numbering.n_max = 1711;
            break;

        case 10:
            absolute_channel_frequency_numbering.n_min = 1806;
            absolute_channel_frequency_numbering.n_max = 1822;
            break;

        case 11:
            absolute_channel_frequency_numbering.n_min = 2142;
            absolute_channel_frequency_numbering.n_max = 2256;
            break;

        case 12:
            absolute_channel_frequency_numbering.n_min = 2374;
            absolute_channel_frequency_numbering.n_max = 2511;
            break;

        case 13:
            absolute_channel_frequency_numbering.n_min = 3126;
            absolute_channel_frequency_numbering.n_max = 3183;
            break;

        case 14:
            absolute_channel_frequency_numbering.n_min = 3184;
            absolute_channel_frequency_numbering.n_max = 3298;
            break;

        case 15:
            absolute_channel_frequency_numbering.n_min = 3299;
            absolute_channel_frequency_numbering.n_max = 3356;
            break;

        case 16:
            absolute_channel_frequency_numbering.n_min = 3994;
            absolute_channel_frequency_numbering.n_max = 4103;
            break;

        case 17:
            absolute_channel_frequency_numbering.n_min = 4392;
            absolute_channel_frequency_numbering.n_max = 4466;
            break;

        case 18:
            absolute_channel_frequency_numbering.n_min = 4105;
            absolute_channel_frequency_numbering.n_max = 4203;
            break;

        case 19:
        default:
            absolute_channel_frequency_numbering.n_min = 4265;
            absolute_channel_frequency_numbering.n_max = 4391;
            break;
    }

    return absolute_channel_frequency_numbering;
}

center_frequency_t get_center_frequency(
    const absolute_channel_frequency_numbering_t absolute_channel_frequency_numbering,
    const uint32_t n) {
    dectnrp_assert(1 <= absolute_channel_frequency_numbering.band_number &&
                       absolute_channel_frequency_numbering.band_number <= 19,
                   "Incorrect band number");

    dectnrp_assert(n < absolute_channel_frequency_numbering.n_min ||
                       absolute_channel_frequency_numbering.n_max < n,
                   "Incorrect center frequency index n");

    center_frequency_t center_frequency{
        .absolute_channel_frequency_numbering = absolute_channel_frequency_numbering, .n = n};

    int64_t channel_offset = 0;

    // check if band number is valid
    if (1 <= absolute_channel_frequency_numbering.band_number &&
        absolute_channel_frequency_numbering.band_number <= 12) {
        center_frequency.F0_i = 450144000;
        center_frequency.channel_spacing_i = 864000;
    } else if (13 <= absolute_channel_frequency_numbering.band_number &&
               absolute_channel_frequency_numbering.band_number <= 16) {
        center_frequency.F0_i = 3000596000;
        center_frequency.channel_spacing_i = 1728000;
        channel_offset = 2952;
    } else {
        center_frequency.F0_i = 5150000000;
        center_frequency.channel_spacing_i = 2000000;
        channel_offset = 4104;
    }

    center_frequency.FC_i = center_frequency.F0_i + (static_cast<int64_t>(n) - channel_offset) *
                                                        center_frequency.channel_spacing_i;

    center_frequency.F0 = static_cast<double>(center_frequency.F0_i);
    center_frequency.channel_spacing = static_cast<double>(center_frequency.channel_spacing_i);
    center_frequency.FC = static_cast<double>(center_frequency.FC_i);

    return center_frequency;
}

reference_time_accuracy_t get_reference_time_accuracy(const uint32_t reference_time_category,
                                                      const bool extreme_condition) {
    dectnrp_assert(reference_time_category == 1 || reference_time_category == 2,
                   "Reference time category must be 1 or 2");

    reference_time_accuracy_t reference_time_accuracy{.reference_time_category =
                                                          reference_time_category};

    if (reference_time_category == 1) {
        reference_time_accuracy.accuracy_ppm = 10;
    } else {
        reference_time_accuracy.accuracy_ppm = 25;
    }

    reference_time_accuracy.extreme_condition = extreme_condition;

    if (extreme_condition) {
        reference_time_accuracy.accuracy_ppm += 5;
    }

    return reference_time_accuracy;
}

bool is_absolute_channel_number_in_range(const uint32_t channel_number) {
    // absolute channel numbers are signalled with 13-bit number
    if (channel_number > common::adt::bitmask_lsb<13>()) {
        return false;
    }

    // check whether absolute channel number is in range according to Table 5.4.2-1
    for (uint32_t band_number = 1; band_number <= 19; ++band_number) {
        auto absolute_channel_frequency_numbering =
            get_absolute_channel_frequency_numbering(band_number);

        if (absolute_channel_frequency_numbering.n_min <= channel_number &&
            channel_number <= absolute_channel_frequency_numbering.n_max) {
            return true;
        }
    }

    return false;
}

}  // namespace dectnrp::section2
