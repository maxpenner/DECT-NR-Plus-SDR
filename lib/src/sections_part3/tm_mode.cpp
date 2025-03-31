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

#include "dectnrp/sections_part3/tm_mode.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::section3::tmmode {

tm_mode_t get_tm_mode(const uint32_t index) {
    dectnrp_assert(index <= 11, "tm_mode_index undefined");

    tm_mode_t q;

    switch (q.index = index) {
        case 0:
            q.N_eff_TX = 1;
            q.N_SS = 1;
            q.cl = false;
            q.N_TS = 1;
            q.N_TX = 1;
            break;

        case 1:
            q.N_eff_TX = 2;
            q.N_SS = 1;
            q.cl = false;
            q.N_TS = 2;
            q.N_TX = 2;
            break;

        case 2:
            q.N_eff_TX = 2;
            q.N_SS = 2;
            q.cl = false;
            q.N_TS = 2;
            q.N_TX = 2;
            break;

        case 3:
            q.N_eff_TX = 1;
            q.N_SS = 1;
            q.cl = true;
            q.N_TS = 1;
            q.N_TX = 2;
            break;

        case 4:
            q.N_eff_TX = 2;
            q.N_SS = 2;
            q.cl = true;
            q.N_TS = 2;
            q.N_TX = 2;
            break;

        case 5:
            q.N_eff_TX = 4;
            q.N_SS = 1;
            q.cl = false;
            q.N_TS = 4;
            q.N_TX = 4;
            break;

        case 6:
            q.N_eff_TX = 4;
            q.N_SS = 4;
            q.cl = false;
            q.N_TS = 4;
            q.N_TX = 4;
            break;

        case 7:
            q.N_eff_TX = 1;
            q.N_SS = 1;
            q.cl = true;
            q.N_TS = 1;
            q.N_TX = 4;
            break;

        case 8:
            q.N_eff_TX = 2;
            q.N_SS = 2;
            q.cl = true;
            q.N_TS = 2;
            q.N_TX = 4;
            break;

        case 9:
            q.N_eff_TX = 4;
            q.N_SS = 4;
            q.cl = true;
            q.N_TS = 4;
            q.N_TX = 4;
            break;

        case 10:
            q.N_eff_TX = 8;
            q.N_SS = 1;
            q.cl = false;
            q.N_TS = 8;
            q.N_TX = 8;
            break;

        case 11:
            q.N_eff_TX = 8;
            q.N_SS = 8;
            q.cl = false;
            q.N_TS = 8;
            q.N_TX = 8;
            break;
    }

    return q;
}

uint32_t get_max_tm_mode_index_depending_on_N_TX(const uint32_t N_TX) {
    dectnrp_assert(N_TX == 1 || N_TX == 2 || N_TX == 4 || N_TX == 8, "N_TX undefined");

    switch (N_TX) {
        case 1:
            return 0;
        case 2:
            return 4;
        case 4:
            return 9;
    }

    return 11;
}

// TX diversity mode for a given number of antennas
uint32_t get_tx_div(const uint32_t N_TX) {
    dectnrp_assert(N_TX == 1 || N_TX == 2 || N_TX == 4 || N_TX == 8, "N_TX undefined");

    switch (N_TX) {
        case 1:
            return 0;
        case 2:
            return 1;
        case 4:
            return 5;
    }

    return 10;
}

uint32_t get_equivalent_tm_mode(const uint32_t N_eff_TX, const uint32_t N_SS) {
    dectnrp_assert(N_eff_TX == 1 || N_eff_TX == 2 || N_eff_TX == 4 || N_eff_TX == 8,
                   "N_eff_TX undefined");
    dectnrp_assert(N_SS > 0, "N_SS undefined");

    switch (N_eff_TX) {
        case 1:
            return 0;
        case 2:
            return N_SS == 1 ? 1 : 2;
        case 4:
            return N_SS == 1 ? 5 : 6;
    }

    return N_SS == 1 ? 10 : 11;
}

}  // namespace dectnrp::section3::tmmode
