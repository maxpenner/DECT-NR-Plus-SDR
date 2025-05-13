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

#include "dectnrp/common/adt/callbacks.hpp"

#include <cstdint>
#include <cstdlib>

using namespace dectnrp;

class argument_t {
    public:
        uint8_t d{};
};

class dummy_t {
    public:
        void cb0([[maybe_unused]] const int64_t now_64,
                 [[maybe_unused]] const size_t idx,
                 [[maybe_unused]] int64_t& next_64,
                 [[maybe_unused]] int64_t& period_64) {
            ++cnt;
        };

        void cb1([[maybe_unused]] const int64_t now_64) { ++cnt; };

        void cb2([[maybe_unused]] const int64_t now_64,
                 [[maybe_unused]] const size_t idx,
                 [[maybe_unused]] int64_t& next_64,
                 [[maybe_unused]] int64_t& period_64,
                 argument_t& argument) {
            ++cnt;
            argument.d += 10;
        };

        uint32_t cnt;
};

static argument_t argument;
static dummy_t d0, d1, d2;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    common::adt::callbacks_t<void> callbacks0;
    common::adt::callbacks_t<void, argument_t&> callbacks1;

    callbacks0.add_callback(std::bind(&dummy_t::cb0,
                                      &d0,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      std::placeholders::_4),
                            100,
                            10);

    callbacks0.add_callback(std::bind(&dummy_t::cb1, &d1, std::placeholders::_1), 100, 4);

    callbacks1.add_callback(std::bind(&dummy_t::cb2,
                                      &d2,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      std::placeholders::_4,
                                      std::placeholders::_5),
                            99,
                            20);

    int64_t now_64 = 0;

    bool any_error = false;

    callbacks0.run(now_64);
    callbacks1.run(now_64, argument);

    callbacks0.run(99);
    callbacks1.run(99, argument);

    callbacks0.run(100);
    callbacks1.run(100, argument);

    if (d0.cnt != 1) {
        any_error = true;
    }

    if (d1.cnt != 1) {
        any_error = true;
    }

    if (d2.cnt != 1 || argument.d != 10) {
        any_error = true;
    }

    callbacks0.run(101);
    callbacks1.run(101, argument);

    callbacks0.run(105);
    callbacks1.run(105, argument);

    callbacks0.run(111);
    callbacks1.run(111, argument);

    if (d0.cnt != 2) {
        any_error = true;
    }

    if (d1.cnt != 3) {
        any_error = true;
    }

    if (d2.cnt != 1 || argument.d != 10) {
        any_error = true;
    }

    callbacks1.run(125, argument);

    if (d2.cnt != 2 || argument.d != 20) {
        any_error = true;
    }

    if (any_error) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
