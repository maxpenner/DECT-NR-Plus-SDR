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

#include "dectnrp/apps/udp.hpp"

#include <cstdlib>
#include <memory>

#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/common/thread/watch.hpp"

using namespace dectnrp;

static uint8_t buffer_tx[1500];
static uint8_t buffer_rx[1500];

int main(int argc, char** argv) {
    application::udp_t udp;

    udp.add_connection_tx("127.0.0.1", 8080);
    udp.add_connection_rx("127.0.0.1", 8080, 10000);

    const std::size_t N = 3;

    for (std::size_t i = 0; i < N; ++i) {
        const auto tx = udp.tx(0, buffer_tx, 123 + i);
        dectnrp_print_inf("tx {}", tx);
    }

    const int64_t tx_time_64 =
        common::watch_t::get_elapsed_since_epoch<int64_t, common::micro, common::steady_clock>();

    const auto tx =
        udp.tx_timed<common::micro, common::steady_clock>(0, buffer_tx, 234, tx_time_64 + 5);

    dectnrp_print_inf("tx {}", tx);

    common::watch_t::sleep(100000);

    for (std::size_t i = 0; i < N + 2; ++i) {
        const auto rx = udp.rx(0, buffer_rx, sizeof(buffer_rx));
        dectnrp_print_inf("rx {}", rx);
    }

    return EXIT_SUCCESS;
}
