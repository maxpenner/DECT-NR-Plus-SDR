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

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <vector>

#include "dectnrp/apps/stream.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/common/thread/threads.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "header_only/argparse/argparse.hpp"

// ctrl+c
static std::atomic<bool> ctrl_c_pressed{false};
static void signal_handler([[maybe_unused]] int signo) {
    ctrl_c_pressed.store(true, std::memory_order_release);
}

// threading
static pthread_t udp_thread;

// individual streams
static std::vector<dectnrp::apps::stream_t> streams;

// stream coordination
static int64_t slowdown_factor_64{};
static int64_t offset_sec_64{};
static int64_t timeadvance_us_64{};

static void* udp_thread_routine([[maybe_unused]] void* ptr) {
    // create vector next times of each stream
    std::vector<int64_t> next_vec(streams.size());

    // determine start time in the near future
    const int64_t start_sec_64 =
        dectnrp::common::watch_t::get_elapsed_since_epoch<int64_t,
                                                          dectnrp::common::seconds,
                                                          dectnrp::common::tai_clock>() +
        offset_sec_64;

    const int64_t start_us_64 = start_sec_64 * dectnrp::apps::stream_t::mega;

    // initialize all streams
    for (std::size_t i = 0; i < streams.size(); i++) {
        next_vec.at(i) = streams.at(i).set_start_full_sec(start_sec_64);
    }

    while (1) {
        // find next stream to transmit
        const std::size_t idx =
            std::distance(next_vec.begin(), std::min_element(next_vec.begin(), next_vec.end()));

        // target time for stream
        const int64_t next_64 = next_vec.at(idx);

        dectnrp_assert(start_us_64 <= next_64, "time out-of-order");

        // target time for stream
        const int64_t next_slowdown_64 = (next_64 - start_us_64) * slowdown_factor_64 + start_us_64;

        const int64_t now_64 = dectnrp::common::watch_t::
            get_elapsed_since_epoch<int64_t, dectnrp::common::micro, dectnrp::common::tai_clock>();

        dectnrp_assert(now_64 < next_slowdown_64, "time out-of-order");

        // do we have to sleep?
        if (now_64 < next_slowdown_64 - timeadvance_us_64) {
            dectnrp::common::watch_t::sleep_until<dectnrp::common::micro,
                                                  dectnrp::common::tai_clock>(next_slowdown_64 -
                                                                              timeadvance_us_64);
        }

        dectnrp_assert(
            (dectnrp::common::watch_t::get_elapsed_since_epoch<int64_t,
                                                               dectnrp::common::micro,
                                                               dectnrp::common::tai_clock>()) <
                next_slowdown_64,
            "time out-of-order");

        // abort prematurely
        if (ctrl_c_pressed.load(std::memory_order_acquire)) {
            break;
        }

        // transmit stream and set next time
        streams.at(idx).generate_payload(nullptr, 0);
        next_vec.at(idx) = streams.at(idx).get_next_us();
    }

    return nullptr;
}

int main(int argc, char** argv) {
    // register signal handler
    signal(SIGINT, signal_handler);

    dectnrp_print_inf("sync started at: {}", dectnrp::common::watch_t::get_date_and_time());

    // create parser
    argparse::ArgumentParser argparse("rtt");

    argparse.add_argument("-s", "--slowdown")
        .default_value(int64_t{1})
        .help("transmission interval in us")
        .scan<'i', int64_t>();

    argparse.add_argument("-o", "--offset")
        .default_value(int64_t{2})
        .help("start offset in seconds")
        .scan<'i', int64_t>();

    argparse.add_argument("-t", "--timeadvance")
        .default_value(int64_t{500})
        .help("preparation deadline for each stream")
        .scan<'i', int64_t>();

    try {
        argparse.parse_args(argc, argv);
    } catch (const std::exception& err) {
        dectnrp_assert_failure("unable to parse arguments");
        return EXIT_FAILURE;
    }

    slowdown_factor_64 = argparse.get<int64_t>("-s");
    offset_sec_64 = argparse.get<int64_t>("-o");
    timeadvance_us_64 = argparse.get<int64_t>("-t");

    dectnrp_assert(1 <= slowdown_factor_64, "slowdown_factor ill-defined");
    dectnrp_assert(slowdown_factor_64 <= 1000, "slowdown_factor ill-defined");

    dectnrp_assert(2 <= offset_sec_64, "offset_sec ill-defined");
    dectnrp_assert(offset_sec_64 <= 10, "offset_sec ill-defined");

    dectnrp_assert(100 <= timeadvance_us_64, "timeadvance_us ill-defined");
    dectnrp_assert(timeadvance_us_64 <= 1000, "timeadvance_us ill-defined");

    // core and priority of thread, start with sudo if elevated
    dectnrp::common::threads_core_prio_config_t threads_core_prio_config;
    threads_core_prio_config.prio_offset = SYNC_RUN_WITH_THREAD_PRIORITY_OFFSET;
    threads_core_prio_config.cpu_core = SYNC_RUN_ON_CORE;

    // add some default streams
    streams.push_back(dectnrp::apps::stream_t(0, 0.001, 0.0000, 1000));
    streams.push_back(dectnrp::apps::stream_t(1, 0.002, 0.0005, 500));
    streams.push_back(dectnrp::apps::stream_t(2, 0.010, 0.0025, 100));
    streams.push_back(dectnrp::apps::stream_t(3, 0.020, 0.0050, 50));

    // start thread
    if (!dectnrp::common::threads_new_rt_mask_custom(
            &udp_thread, &udp_thread_routine, NULL, threads_core_prio_config)) {
        dectnrp_print_wrn("Unable to start udp_thread.");
        return EXIT_FAILURE;
    }

    // ctrl+c
    while (!ctrl_c_pressed.load(std::memory_order_acquire)) {
        dectnrp::common::watch_t::sleep<dectnrp::common::milli>(250);
    }

    pthread_join(udp_thread, NULL);

    for (std::size_t i = 0; i < streams.size(); i++) {
        dectnrp_print_inf("Index: {} tx: {}", i, streams.at(i).get_stats().tx);
    }

    dectnrp_print_inf("sync stopped at: {}", dectnrp::common::watch_t::get_date_and_time());

    return EXIT_SUCCESS;
}
