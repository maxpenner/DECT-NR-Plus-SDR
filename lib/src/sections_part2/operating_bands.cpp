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

#include "dectnrp/sections_part2/operating_bands.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp2 {

operating_band_t get_operating_band(const uint32_t band_number) {
    dectnrp_assert(1 <= band_number && band_number <= 17, "Band number must be between 1 and 17");

    operating_band_t operating_band;

    switch (operating_band.band_number = band_number) {
        case 1:
            operating_band.f_low = 1880.0;
            operating_band.f_high = 1900.0;
            break;

        case 2:
            operating_band.f_low = 1900.0;
            operating_band.f_high = 1920.0;
            break;

        case 3:
            operating_band.f_low = 2400.0;
            operating_band.f_high = 2483.5;
            break;

        case 4:
            operating_band.f_low = 902.0;
            operating_band.f_high = 928.0;
            break;

        case 5:
            operating_band.f_low = 450.0;
            operating_band.f_high = 470.0;
            break;

        case 6:
            operating_band.f_low = 698.0;
            operating_band.f_high = 806.0;
            break;

        case 7:
            operating_band.f_low = 716.0;
            operating_band.f_high = 728.0;
            break;

        case 8:
            operating_band.f_low = 1432.0;
            operating_band.f_high = 1517.0;
            break;

        case 9:
            operating_band.f_low = 1910.0;
            operating_band.f_high = 1930.0;
            break;

        case 10:
            operating_band.f_low = 2010.0;
            operating_band.f_high = 2025.0;
            break;

        case 11:
            operating_band.f_low = 2300.0;
            operating_band.f_high = 2400.0;
            break;

        case 12:
            operating_band.f_low = 2500.0;
            operating_band.f_high = 2620.0;
            break;

        case 13:
            operating_band.f_low = 3300.0;
            operating_band.f_high = 3400.0;
            break;

        case 14:
            operating_band.f_low = 3400.0;
            operating_band.f_high = 3600.0;
            break;

        case 15:
            operating_band.f_low = 3600.0;
            operating_band.f_high = 3700.0;
            break;

        case 16:
            operating_band.f_low = 4800.0;
            operating_band.f_high = 4990.0;
            break;

        case 17:
        default:
            operating_band.f_low = 5725.0;
            operating_band.f_high = 5875.0;
            break;
    }

    operating_band.f_low = operating_band.f_low * 1e6;
    operating_band.f_high = operating_band.f_high * 1e6;

    return operating_band;
}

}  // namespace dectnrp::sp2
