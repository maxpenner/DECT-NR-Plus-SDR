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

#include "dectnrp/common/ant.hpp"

#include <cstdlib>

using namespace dectnrp;

int main(int argc, char** argv) {
    common::ant_t ant0(1);

    if (ant0.get_nof_antennas() != 1) {
        return EXIT_FAILURE;
    }

    if (ant0.has_any_larger(0.0f)) {
        return EXIT_FAILURE;
    }

    const float f0 = 10.0f;

    ant0.at(0) = f0;

    if (!ant0.has_any_larger(0.0f)) {
        return EXIT_FAILURE;
    }

    if (ant0.get_max() != f0) {
        return EXIT_FAILURE;
    }

    if (ant0.get_index_of_max() != 0) {
        return EXIT_FAILURE;
    }

    if (ant0.get_sum() != f0) {
        return EXIT_FAILURE;
    }

    if (ant0.get_nof_larger(0.0f) != 1) {
        return EXIT_FAILURE;
    }

    if (ant0.get_nof_larger(10.1f) != 0) {
        return EXIT_FAILURE;
    }

    common::ant_t ant1(4);

    if (ant1.get_nof_antennas() != 4) {
        return EXIT_FAILURE;
    }

    if (ant1.has_any_larger(0.0f)) {
        return EXIT_FAILURE;
    }

    const float f1 = 1.0f;
    const float f2 = -1.0f;
    const float f3 = 2.0f;
    const float f4 = 0.3f;
    ant1.at(0) = f1;
    ant1.at(1) = f2;
    ant1.at(2) = f3;
    ant1.at(3) = f4;

    if (ant1.get_max() != f3) {
        return EXIT_FAILURE;
    }

    if (!ant1.has_any_larger(0.0f)) {
        return EXIT_FAILURE;
    }

    if (ant1.get_index_of_max() != 2) {
        return EXIT_FAILURE;
    }

    if (ant1.get_sum() != f1 + f2 + f3 + f4) {
        return EXIT_FAILURE;
    }

    if (ant1.get_nof_larger(0.0f) != 3) {
        return EXIT_FAILURE;
    }

    if (ant1.get_nof_larger(-2.0f) != 4) {
        return EXIT_FAILURE;
    }

    if (ant1.get_nof_larger(2.1f) != 0) {
        return EXIT_FAILURE;
    }

    common::ant_t ant2(8);

    if (ant2.get_nof_antennas() != 8) {
        return EXIT_FAILURE;
    }

    if (ant2.has_any_larger(0.0f)) {
        return EXIT_FAILURE;
    }

    ant2.at(0) = f1;

    if (!ant2.has_any_larger(0.0f)) {
        return EXIT_FAILURE;
    }

    ant2.at(1) = f2;
    ant2.at(2) = f3;
    ant2.at(3) = f4;

    if (ant2.at(4) != 0.0f || ant2.at(5) != 0.0f || ant2.at(6) != 0.0f || ant2.at(7) != 0.0f) {
        return EXIT_FAILURE;
    }

    if (ant2.get_max() != f3) {
        return EXIT_FAILURE;
    }

    if (ant2.get_index_of_max() != 2) {
        return EXIT_FAILURE;
    }

    if (ant2.get_sum() != f1 + f2 + f3 + f4) {
        return EXIT_FAILURE;
    }

    if (ant2.get_nof_larger(0.0f) != 3) {
        return EXIT_FAILURE;
    }

    if (ant2.get_nof_larger(-2.0f) != 8) {
        return EXIT_FAILURE;
    }

    if (ant2.get_nof_larger(2.1f) != 0) {
        return EXIT_FAILURE;
    }

    common::ant_t ant3(7);

    ant3.fill(1.0f);

    if (ant3.get_sum() != 7.0f) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
