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

#include "sync.hpp"

#include <atomic>
#include <csignal>
#include <cstdint>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/external/argparse/argparse.hpp"

// ctrl+c
static std::atomic<bool> ctrl_c_pressed{false};
static void signal_handler(int signo) { ctrl_c_pressed.store(true); }

int main(int argc, char** argv) {
    // register signal handler
    signal(SIGINT, signal_handler);

    // create parser
    argparse::ArgumentParser argparse("rtt");
    argparse.add_argument("-i", "--interval")
        .default_value(int64_t{0})
        .help("transmission interval in us")
        .scan<'i', int64_t>();

    try {
        argparse.parse_args(argc, argv);
    } catch (const std::exception& err) {
        dectnrp_assert_failure("unable to parse arguments");
        return EXIT_FAILURE;
    }

    auto interval = argparse.get<int64_t>("-i");

    dectnrp_print_inf("interval {}", interval);

    return EXIT_SUCCESS;
}
