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

#include "dectnrp/common/adt/miscellaneous.hpp"

using namespace dectnrp;

int test0() {
    if (common::adt::ceil_divide_integer(10ULL, 1ULL) != 10) {
        return EXIT_FAILURE;
    }

    if (common::adt::ceil_divide_integer(10ULL, 2ULL) != 5) {
        return EXIT_FAILURE;
    }

    if (common::adt::ceil_divide_integer(10ULL, 5ULL) != 2) {
        return EXIT_FAILURE;
    }

    if (common::adt::ceil_divide_integer(10ULL, 6ULL) != 2) {
        return EXIT_FAILURE;
    }

    if (common::adt::ceil_divide_integer(10ULL, 9ULL) != 2) {
        return EXIT_FAILURE;
    }

    if (common::adt::ceil_divide_integer(10ULL, 10ULL) != 1) {
        return EXIT_FAILURE;
    }

    if (common::adt::ceil_divide_integer(10ULL, 11ULL) != 1) {
        return EXIT_FAILURE;
    }

    if (common::adt::ceil_divide_integer(10ULL, 10000000ULL) != 1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int test1() {
    if (common::adt::round_integer(10ULL, 1ULL) != 10) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 2ULL) != 5) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 3ULL) != 3) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 4ULL) != 3) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 5ULL) != 2) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 6ULL) != 2) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 7ULL) != 1) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 20ULL) != 1) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10ULL, 21ULL) != 0) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(10LL, -1LL) != -10) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(-10LL, 1LL) != -10) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(-10LL, -1LL) != 10) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(-10LL, -20LL) != 1) {
        return EXIT_FAILURE;
    }

    if (common::adt::round_integer(-10LL, -21LL) != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    if (test0() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    if (test1() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
