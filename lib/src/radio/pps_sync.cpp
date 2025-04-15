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

#include "dectnrp/radio/pps_sync.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/radio/hw.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/radio/pps_sync_param.hpp"

namespace dectnrp::radio {

void pps_sync_t::sync_procedure(hw_t& hw) {
    common::watch_t watch_dog;

    {
        std::unique_lock<std::mutex> lk(mtx);

        dectnrp_assert(nof_hw > 0, "number of hw must be larger zero");
        dectnrp_assert(hw.id < nof_hw, "id too large");

        ++nof_hw_cnt;

        // last one?
        if (nof_hw_cnt == nof_hw) {
            // last device to register blocks until a PPS occurred
            hw.pps_wait_for_next();

            // unblock all other hw
            cv.notify_all();
        } else {
            // wait for other instances
            while (nof_hw_cnt != nof_hw) {
                cv.wait_for(lk, std::chrono::milliseconds(CV_WAIT_TIMEOUT_MS));

                dectnrp_assert(!watch_dog.is_elapsed<common::milli>(CV_WAIT_WATCHDOG_MS),
                               "waiting too long for hw to synchronize PPS");
            }
        }
    }

    // wait for another PPS in case instances of hw do not share a common PPS signal
    hw.pps_wait_for_next();

#ifdef RADIO_PPS_SYNC_SYNC_TO_TAI_OR_TO_ZERO
    // sleep for a short time so that the full second of the operating system has certainly passed
    common::watch_t::sleep<common::milli>(50);

    // get full second, but set to value + 1 at next PPS
    hw.pps_full_sec_at_next(
        common::watch_t::get_elapsed_since_epoch<int64_t, common::seconds, common::tai_clock>() +
        int64_t{1});
#else
    hw.pps_full_sec_at_next(0);
#endif

    // applies only for real hardware
    if (dynamic_cast<radio::hw_simulator_t*>(&hw) == nullptr) {
        // sleep until every hw has definitely heard a PPS and set its internal time to zero
        common::watch_t::sleep<common::milli>(1500);
    }
}

}  // namespace dectnrp::radio
