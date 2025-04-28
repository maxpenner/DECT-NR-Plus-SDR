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

#include "rtt.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <string>
#include <vector>

// UDP networking
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

// json export
#include <fstream>
#include <sstream>

#include "dectnrp/apps/udp.hpp"
#include "dectnrp/common/json/json_export.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/common/thread/threads.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/external/nlohmann/json.hpp"

// ctrl+c
static std::atomic<bool> ctrl_c_pressed{false};
static void signal_handler(int signo) { ctrl_c_pressed.store(true); }

// threading
static pthread_t udp_thread;

// UDP
static dectnrp::apps::udp_t udp;
static constexpr std::size_t udp_data_idx{0};
static constexpr std::size_t udp_print_idx{1};

// UDP TX and RX buffer
static uint8_t tx_buffer[TFW_RTT_TX_LENGTH_MAXIMUM_BYTE];
static uint8_t rx_buffer[TFW_RTT_TX_LENGTH_MAXIMUM_BYTE];

// json file export
static size_t json_file_cnt{0};

// result of a single measurement
struct measurement_results_t {
        size_t length{0};
        int64_t rtt_mac2mac_64{-1};
};

static measurement_results_t send_random_packet_to_sdr_and_await_response(const size_t n_byte) {
    dectnrp_assert(TFW_RTT_TX_LENGTH_MINIMUM_BYTE <= n_byte, "message too small");
    dectnrp_assert(n_byte <= TFW_RTT_TX_LENGTH_MAXIMUM_BYTE, "message too large");

    // fill required part of TX message with random data
    for (size_t i = 0; i < n_byte; ++i) {
        tx_buffer[i] = std::rand() % 256;
    }

    // send packet to SDR and ...
    udp.tx(udp_data_idx, tx_buffer, n_byte);

    // ... immediately wait for response
    const ssize_t ret = udp.rx(udp_data_idx, rx_buffer, TFW_RTT_TX_LENGTH_MAXIMUM_BYTE);

    // return value
    measurement_results_t measurement_results;

    // did we receive a response?
    if (0 < ret) {
        // transfer length to return value
        measurement_results.length = ret;

        // is the response of the same size?
        if (measurement_results.length == n_byte) {
            // same content?
            for (int i = 0; i < TFW_RTT_TX_VS_RX_VERIFICATION_LENGTH_BYTE; ++i) {
                if (tx_buffer[i] != rx_buffer[i]) {
                    dectnrp_print_wrn("RX message has different content {} {} {}",
                                      i,
                                      int(tx_buffer[i]),
                                      int(rx_buffer[i]));
                    break;
                }
            }

            // extract the mac2mac rtt
            memcpy((char*)&measurement_results.rtt_mac2mac_64,
                   &rx_buffer[TFW_RTT_TX_VS_RX_VERIFICATION_LENGTH_BYTE],
                   sizeof(measurement_results.rtt_mac2mac_64));
        } else {
            dectnrp_print_wrn(
                "RX message has different size {} {}. Probe?", n_byte, measurement_results.length);
        }
    } else {
        dectnrp_print_wrn("Timeout!");
    }

    return measurement_results;
}

static size_t probe_sdr_packet_length() {
    // result of our probe test
    measurement_results_t measurement_results;

    while (1) {
        measurement_results =
            send_random_packet_to_sdr_and_await_response(TFW_RTT_TX_LENGTH_MINIMUM_BYTE);

        if (0 < measurement_results.length) {
            dectnrp_assert(TFW_RTT_TX_LENGTH_MINIMUM_BYTE <= measurement_results.length,
                           "measurement_results.length too small");
            dectnrp_assert(measurement_results.length <= TFW_RTT_TX_LENGTH_MAXIMUM_BYTE,
                           "measurement_results.length too large");

            dectnrp_print_inf("SDR packet length probe: {} byte", measurement_results.length);

            break;
        }

        // abort prematurely
        if (ctrl_c_pressed.load()) {
            break;
        }

        dectnrp::common::watch_t::sleep<dectnrp::common::milli>(100);
    }

    return measurement_results.length;
}

static void analyze_total_time_and_rate(const std::vector<int64_t>& elapsed_ns,
                                        const int64_t elapsed_total_ns) {
    dectnrp_assert(elapsed_ns.size() == RTT_MEASUREMENTS_PER_PRINT,
                   "incorrect number of measurements");

    const int64_t rtt_success =
        std::count_if(elapsed_ns.begin(), elapsed_ns.end(), [](auto elem) { return 0 <= elem; });
    const int64_t rtt_fail =
        std::count_if(elapsed_ns.begin(), elapsed_ns.end(), [](auto elem) { return elem < 0; });

    dectnrp_assert(rtt_success + rtt_fail == RTT_MEASUREMENTS_PER_PRINT,
                   "incorrect sum of success and fail");

    auto ns2us = [](auto a) { return a / decltype(a)(1000); };

    const double time_per_measurement_avg_us =
        ns2us(static_cast<double>(elapsed_total_ns) / static_cast<double>(elapsed_ns.size()));

    // show results
    dectnrp_print_inf(
        "time_per_measurement_avg_us = {:.2f} us measurement_rate = {:.2f} measurements/s",
        time_per_measurement_avg_us,
        1.0e6 / time_per_measurement_avg_us);
    dectnrp_print_inf("rtt_success = {} rtt_fail = {} PER = {}",
                      rtt_success,
                      rtt_fail,
                      static_cast<double>(rtt_fail) / static_cast<double>(elapsed_ns.size()));
}

static void analyze_min_max_mean(const std::string identifier,
                                 std::vector<int64_t> elapsed_ns,
                                 const int64_t elapsed_total_ns) {
    dectnrp_assert(elapsed_ns.size() == RTT_MEASUREMENTS_PER_PRINT,
                   "incorrect number of measurements");

    // remove any negative elements, round-trip did not work
    elapsed_ns.erase(
        std::remove_if(elapsed_ns.begin(), elapsed_ns.end(), [](auto a) { return a < 0; }),
        elapsed_ns.end());

    const int64_t rtt_success = elapsed_ns.size();

    if (rtt_success == 0) {
        dectnrp_print_wrn("no single complete round-trip, cannot determine any stats");
        return;
    }

    // relevant stats
    int64_t rtt_mean_ns = 0;
    int64_t rtt_max_ns = -std::numeric_limits<int64_t>::max() / 2;
    int64_t rtt_min_ns = std::numeric_limits<int64_t>::max() / 2;

    // go over all elements
    for (const auto elem_ns : elapsed_ns) {
        dectnrp_assert(0 < elem_ns, "negative RTT");

        rtt_mean_ns += elem_ns;
        rtt_max_ns = std::max(elem_ns, rtt_max_ns);
        rtt_min_ns = std::min(elem_ns, rtt_min_ns);
    }

    rtt_mean_ns /= rtt_success;

    auto ns2us = [](auto a) { return a / decltype(a)(1000); };

    // convert to microseconds
    const int64_t rtt_min_us = ns2us(rtt_min_ns);
    const int64_t rtt_max_us = ns2us(rtt_max_ns);
    const int64_t rtt_mean_us = ns2us(rtt_mean_ns);

    dectnrp_print_inf("{} rtt_min = {} us rtt_max = {} rtt_mean = {} us rtt_range = {} us",
                      identifier,
                      rtt_min_us,
                      rtt_max_us,
                      rtt_mean_us,
                      rtt_max_us - rtt_min_us);
}

static void save_as_json(const std::string identifier,
                         const std::vector<int64_t>& elapsed_ns,
                         const int64_t elapsed_total_ns) {
    dectnrp_assert(elapsed_ns.size() == RTT_MEASUREMENTS_PER_PRINT,
                   "incorrect number of measurements");

    // create filename with unique file count
    std::ostringstream filename;
    filename << "rtt_external_" << identifier << "_" << std::setw(10) << std::setfill('0')
             << json_file_cnt;

    nlohmann::ordered_json j_packet_data;
    j_packet_data["elapsed_ns"] = elapsed_ns;
    j_packet_data["elapsed_total_ns"] = elapsed_total_ns;

    dectnrp::common::json_export_t::write_to_disk(j_packet_data, filename.str());
}

static void* udp_thread_routine(void* ptr) {
    const size_t sdr_packet_length = probe_sdr_packet_length();

    if (sdr_packet_length == 0) {
        return nullptr;
    }

    // send small packet to SDR on other port to indicate measurement is finished
    udp.tx(udp_print_idx, tx_buffer, TFW_RTT_TX_LENGTH_MINIMUM_BYTE);

    // give SDR some time to finish print
    dectnrp::common::watch_t::sleep<dectnrp::common::milli>(500);

    while (true) {
        // preallocate results container
        std::vector<int64_t> elapsed_mac2mac_ns(RTT_MEASUREMENTS_PER_PRINT, -1);
        std::vector<int64_t> elapsed_rtt2rtt_ns(RTT_MEASUREMENTS_PER_PRINT, -1);

        // tic
        dectnrp::common::watch_t watch_all_measurements;

        size_t N_measurements_cnt = 0;
        while (N_measurements_cnt < RTT_MEASUREMENTS_PER_PRINT) {
            // abort prematurely
            if (ctrl_c_pressed.load()) {
                return nullptr;
            }

            // tic
            dectnrp::common::watch_t watch;

            const measurement_results_t measurement_results =
                send_random_packet_to_sdr_and_await_response(sdr_packet_length);

            if (0 < measurement_results.length) {
                // toc
                elapsed_mac2mac_ns.at(N_measurements_cnt) = measurement_results.rtt_mac2mac_64;
                elapsed_rtt2rtt_ns.at(N_measurements_cnt) = watch.get_elapsed();
            }

            ++N_measurements_cnt;

            if (0 < RTT_MEASUREMENT_TO_MEASUREMENT_SLEEP_US) {
                dectnrp::common::watch_t::sleep<dectnrp::common::micro>(
                    RTT_MEASUREMENT_TO_MEASUREMENT_SLEEP_US);
            }
        }

        // toc
        const int64_t elapsed_total_ns = watch_all_measurements.get_elapsed();

        // wait some time before starting a new measurement
        dectnrp::common::watch_t::sleep<dectnrp::common::milli>(1000);

        // send small packet to SDR on other port to indicate measurement is finished
        udp.tx(udp_print_idx, tx_buffer, TFW_RTT_TX_LENGTH_MINIMUM_BYTE);

        // separator between two runs
        dectnrp_print_inf("");

        // analyse measurement and show results
        analyze_total_time_and_rate(elapsed_mac2mac_ns, elapsed_total_ns);
        analyze_min_max_mean("mac2mac", elapsed_mac2mac_ns, elapsed_total_ns);
        analyze_min_max_mean("rtt2rtt", elapsed_rtt2rtt_ns, elapsed_total_ns);
        save_as_json("mac2mac", elapsed_mac2mac_ns, elapsed_total_ns);
        save_as_json("rtt2rtt", elapsed_rtt2rtt_ns, elapsed_total_ns);

        // increment file count
        ++json_file_cnt;

        // wait some time before starting a new measurement
        dectnrp::common::watch_t::sleep<dectnrp::common::milli>(2000);
    }

    return nullptr;
}

int main(int argc, char** argv) {
    dectnrp_assert(TFW_RTT_TX_VS_RX_VERIFICATION_LENGTH_BYTE <= TFW_RTT_TX_LENGTH_MAXIMUM_BYTE,
                   "verification length must be smaller than transmission length");

    // register signal handler
    signal(SIGINT, signal_handler);

    std::srand(std::time(nullptr));

    memset(tx_buffer, 0x0, TFW_RTT_TX_LENGTH_MAXIMUM_BYTE);
    memset(rx_buffer, 0x0, TFW_RTT_TX_LENGTH_MAXIMUM_BYTE);

    udp.add_connection_tx("127.0.0.1", TFW_RTT_UDP_PORT_DATA);
    udp.add_connection_tx("127.0.0.1", TFW_RTT_UDP_PORT_PRINT);
    udp.add_connection_rx("127.0.0.1", 8050, RTT_UDP_TIMEOUT_BEFORE_ASSUMING_ERROR_US);

    // core and priority of thread, start with sudo if elevated
    dectnrp::common::threads_core_prio_config_t threads_core_prio_config;
    threads_core_prio_config.prio_offset = RTT_RUN_WITH_THREAD_PRIORITY_OFFSET;
    threads_core_prio_config.cpu_core = RTT_RUN_ON_CORE;

    // start thread
    if (!dectnrp::common::threads_new_rt_mask_custom(
            &udp_thread, &udp_thread_routine, NULL, threads_core_prio_config)) {
        dectnrp_print_wrn("Unable to start udp_thread.");
        return EXIT_FAILURE;
    }

    // ctrl+c
    while (!ctrl_c_pressed.load()) {
        dectnrp::common::watch_t::sleep<dectnrp::common::milli>(250);
    }

    pthread_join(udp_thread, NULL);

    return EXIT_SUCCESS;
}
