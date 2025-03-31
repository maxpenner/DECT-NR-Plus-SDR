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

#include "dectnrp/common/thread/watch.hpp"

#include "dectnrp/common/prog/print.hpp"

using namespace dectnrp;

int main(int argc, char** argv) {
    common::watch_t watch;

    const auto A = watch.get_elapsed_since_epoch<int64_t, common::seconds, common::system_clock>();
    const auto B = watch.get_elapsed_since_epoch<int64_t, common::seconds, common::utc_clock>();
    const auto C = watch.get_elapsed_since_epoch<int64_t, common::seconds, common::tai_clock>();
    const auto D = watch.get_elapsed_since_epoch<int64_t, common::seconds, common::gps_clock>();

    dectnrp_print_inf("SCL {}", A);
    dectnrp_print_inf("UTC {}", B);
    dectnrp_print_inf("TAI {}", C);
    dectnrp_print_inf("GPS {}", D);

    dectnrp_print_inf("UTC - SCL {}", B - A);
    dectnrp_print_inf("TAI - UTC {}", C - B);
    dectnrp_print_inf("epoch_tai_utc_sec {}", watch.epoch_tai_utc_sec.count());
    dectnrp_print_inf("GPS - SCL {}", D - B);
    dectnrp_print_inf("epoch_gps_utc_sec {}", watch.epoch_gps_utc_sec.count());

    const auto E = watch.get_elapsed_since_epoch<int64_t, common::seconds, common::tai_clock>();
    dectnrp_print_inf("seconds sleep start");
    for (size_t i = 1; i < 5; i++) {
        const int64_t plus_sec = i;
        common::watch_t::sleep_until<common::seconds, common::tai_clock>(E + plus_sec);
        dectnrp_print_inf("seconds sleep {}", plus_sec);
    }

    const auto F = watch.get_elapsed_since_epoch<int64_t, common::milli, common::tai_clock>();
    dectnrp_print_inf("milliseconds sleep start");
    for (size_t i = 1; i < 5; i++) {
        const int64_t plus_sec = 100 * i;
        common::watch_t::sleep_until<common::milli, common::tai_clock>(F + plus_sec);
        dectnrp_print_inf("milliseconds sleep {}", plus_sec);
    }

    return EXIT_SUCCESS;
}
