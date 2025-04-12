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

#define ASSERT_BAND_NUMBER(x) dectnrp_assert(1 <= x || x <= 19, "band number {} unknown", x)

namespace dectnrp::section2 {

absolute_channel_frequency_numbering_t get_absolute_channel_frequency_numbering(
    const uint32_t band_number) {
    ASSERT_BAND_NUMBER(band_number);

    // for now assume a default spacing of 2
    absolute_channel_frequency_numbering_t acfn = {.band_number = band_number, .n_spacing = 2};

    switch (band_number) {
        case 1:
            acfn.n_min = 1657;
            acfn.n_max = 1677;
            break;

        case 2:
            acfn.n_min = 1680;
            acfn.n_max = 1700;
            break;

        case 3:
            acfn.n_min = 2258;
            acfn.n_max = 2352;
            break;

        case 4:
            acfn.n_min = 524;
            acfn.n_max = 552;
            break;

        case 5:
            acfn.n_min = 1;
            acfn.n_max = 22;
            break;

        case 6:
            acfn.n_min = 288;
            acfn.n_max = 411;
            break;

        case 7:
            acfn.n_min = 309;
            acfn.n_max = 321;
            break;

        case 8:
            acfn.n_min = 1137;
            acfn.n_max = 1234;
            break;

        case 9:
            acfn.n_min = 1691;
            acfn.n_max = 1711;
            break;

        case 10:
            acfn.n_min = 1806;
            acfn.n_max = 1822;
            break;

        case 11:
            acfn.n_min = 2142;
            acfn.n_max = 2256;
            break;

        case 12:
            acfn.n_min = 2374;
            acfn.n_max = 2511;
            break;

        case 13:
            acfn.n_min = 3126;
            acfn.n_max = 3183;
            break;

        case 14:
            acfn.n_min = 3184;
            acfn.n_max = 3298;
            break;

        case 15:
            acfn.n_min = 3299;
            acfn.n_max = 3356;
            break;

        case 16:
            acfn.n_min = 3994;
            acfn.n_max = 4103;
            break;

        case 17:
            acfn.n_min = 4392;
            acfn.n_max = 4466;
            break;

        case 18:
            acfn.n_min = 4105;
            acfn.n_max = 4203;
            break;

        case 19:
            [[fallthrough]];
        default:
            acfn.n_min = 4265;
            acfn.n_max = 4391;
            break;
    }

    return acfn;
}

center_frequency_t get_center_frequency(const absolute_channel_frequency_numbering_t acfn,
                                        const uint32_t n) {
    ASSERT_BAND_NUMBER(acfn.band_number);

    dectnrp_assert(n < acfn.n_min || acfn.n_max < n, "incorrect center frequency index {}", n);

    center_frequency_t center_frequency{.acfn = acfn, .n = n};

    int64_t channel_offset = 0;

    if (1 <= acfn.band_number && acfn.band_number <= 12) {
        center_frequency.F0_i = 450144000;
        center_frequency.channel_spacing_i = 864000;
    } else if (13 <= acfn.band_number && acfn.band_number <= 16) {
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

center_frequency_t get_center_frequency(const uint32_t band_number, const uint32_t n) {
    return get_center_frequency(get_absolute_channel_frequency_numbering(band_number), n);
}

bool is_absolute_channel_number_in_range(const uint32_t channel_number) {
    // absolute channel numbers are signalled with 13-bit number
    if (channel_number > common::adt::bitmask_lsb<13>()) {
        return false;
    }

    // check whether absolute channel number is in range according to Table 5.4.2-1
    for (uint32_t band_number = 1; band_number <= 19; ++band_number) {
        const auto acfn = get_absolute_channel_frequency_numbering(band_number);

        if (acfn.n_min <= channel_number && channel_number <= acfn.n_max) {
            return true;
        }
    }

    return false;
}

}  // namespace dectnrp::section2
