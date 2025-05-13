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

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <memory>
#include <string>

#include "dectnrp/build_info.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/phy/phy.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/radio/radio.hpp"
#include "dectnrp/radio/radio_config.hpp"
#include "dectnrp/upper/upper.hpp"
#include "dectnrp/upper/upper_config.hpp"

#ifdef ENABLE_ASSERT
#include "dectnrp/common/prog/simd.hpp"
#endif

static std::atomic<bool> ctrl_c_pressed{false};

static void signal_handler([[maybe_unused]] int signo) { ctrl_c_pressed.store(true); }

int main(int argc, char** argv) {
    // register signal handler
    signal(SIGINT, signal_handler);

    // setup logging
    try {
        dectnrp_log_setup("log.txt");
    } catch (std::exception& e) {
        dectnrp_print_wrn("Exception during log setup: {}. Delete the log file log.txt.", e.what());
        return EXIT_FAILURE;
    }

    // log and print start time
    const auto start_time_str{dectnrp::common::watch_t::get_date_and_time()};
    dectnrp_log_inf("dectnrp started at: {}", start_time_str);
    dectnrp_print_inf("dectnrp started at: {}", start_time_str);

    // log compiled version
    dectnrp_log_inf("DECTNRP_VERSION_STRING {}", dectnrp::version::DECTNRP_VERSION_STRING);
    dectnrp_log_inf("DECTNRP_BUILD_MODE {}", dectnrp::version::DECTNRP_BUILD_MODE);
    dectnrp_log_inf("DECTNRP_GIT_INFO {}", dectnrp::version::DECTNRP_GIT_INFO);

    // parse arguments
    dectnrp_assert(argc == 2, "argument must contain one path to folder with configuration files");
    const std::string configuration_directory = argv[1];
    dectnrp_log_inf("configuration_directory {}", configuration_directory);

#ifdef ENABLE_ASSERT
    dectnrp::common::simd::assert_simd_libs_use_same_alignment();
#endif

    // read and check configuration files
    dectnrp::radio::radio_config_t radio_config;
    dectnrp::phy::phy_config_t phy_config;
    dectnrp::upper::upper_config_t upper_config;
    try {
        radio_config = dectnrp::radio::radio_config_t(configuration_directory);
        phy_config = dectnrp::phy::phy_config_t(configuration_directory);
        upper_config = dectnrp::upper::upper_config_t(configuration_directory);
    } catch (std::exception& e) {
        dectnrp_log_wrn("Exception during configuration file loading: {}", e.what());
        return EXIT_FAILURE;
    }

    // init all layers of stack
    std::unique_ptr<dectnrp::radio::radio_t> radio;
    std::unique_ptr<dectnrp::phy::phy_t> phy;
    std::unique_ptr<dectnrp::upper::upper_t> upper;
    try {
        radio = std::make_unique<dectnrp::radio::radio_t>(radio_config);
        phy = std::make_unique<dectnrp::phy::phy_t>(phy_config, *radio);
        upper = std::make_unique<dectnrp::upper::upper_t>(upper_config, *radio, *phy);
    } catch (std::exception& e) {
        dectnrp_log_wrn("Exception during stack initialization: {}", e.what());
        return EXIT_FAILURE;
    }

    // write log file before starting threads
    dectnrp_log_save();

    upper->start_threads_of_all_layer_units();  // start upper layers
    phy->start_threads_of_all_layer_units();    // start PHY layer and get ready to process samples
    radio->start_threads_of_all_layer_units();  // start radio layer and stream samples

    // wait for user to press ctrl+c
    while (!ctrl_c_pressed.load()) {
        dectnrp::common::watch_t::sleep<dectnrp::common::milli>(250);
        dectnrp_log_save();
    }

    dectnrp_log_inf("dectnrp ctrl+c pressed.");
    dectnrp_print_inf("dectnrp ctrl+c pressed.");

    upper->stop_threads_of_all_layer_units();  // gracefully shut down DECTNRP connections
    phy->stop_threads_of_all_layer_units();    // stop processing samples
    radio->stop_threads_of_all_layer_units();  // stop streaming samples, stopped last as many
                                               // components depend on an increasing sample time

    // log and print stop time
    const auto stop_time_str = dectnrp::common::watch_t::get_date_and_time();
    dectnrp_log_inf("dectnrp stopped at: {}", stop_time_str);
    dectnrp_print_inf("dectnrp stopped at: {}", stop_time_str);

    dectnrp_log_save();

    return EXIT_SUCCESS;
}
