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

#include "dectnrp/common/adt/enumeration.hpp"

#include <cstdint>
#include <cstdlib>

enum class t0_t : uint32_t {
    not_defined = 100,
    val0 = 0,
    val1,
    val2,
    val3,
    upper
};

enum class t1_t : uint32_t {
    not_defined = 1000,
    lower = 100,
    val101 = 101,
    val102,
    val103,
    val104,
    upper
};

using namespace dectnrp;

static int test_t0() {
    if (common::adt::from_coded_value<t0_t>(0) != t0_t::val0) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t0_t>(1) != t0_t::val1) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t0_t>(2) != t0_t::val2) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t0_t>(3) != t0_t::val3) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t0_t>(4) != t0_t::not_defined) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t0_t>(12312) != t0_t::not_defined) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int test_t1() {
    if (common::adt::from_coded_value<t1_t>(99) != t1_t::not_defined) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(100) != t1_t::not_defined) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(101) != t1_t::val101) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(102) != t1_t::val102) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(103) != t1_t::val103) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(104) != t1_t::val104) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(105) != t1_t::not_defined) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(106) != t1_t::not_defined) {
        return EXIT_FAILURE;
    }

    if (common::adt::from_coded_value<t1_t>(1000) != t1_t::not_defined) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int test_valid() {
    if (common::adt::is_valid<t1_t>(t1_t::not_defined)) {
        return EXIT_FAILURE;
    }

    if (common::adt::is_valid<t1_t>(t1_t::lower)) {
        return EXIT_FAILURE;
    }

    if (!common::adt::is_valid<t1_t>(t1_t::val101)) {
        return EXIT_FAILURE;
    }

    if (common::adt::is_valid<t1_t>(t1_t::upper)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    if (test_t0() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    if (test_t1() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    if (test_valid() == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
