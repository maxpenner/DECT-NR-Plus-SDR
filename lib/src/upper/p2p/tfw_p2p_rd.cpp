/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/upper/p2p/tfw_p2p_rd.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::upper::tfw::p2p {

phy::irregular_report_t tfw_p2p_rd_t::work_start(const int64_t start_time_64) {
    rd.start_time_iq_streaming_64 = start_time_64;

    dectnrp_assert(tpoint_state != nullptr, "initial state not set");

    return tpoint_state->entry();
}

void tfw_p2p_rd_t::work_stop() {
    // instruct radio device to shut down
    rd.rd_mode.store(rd_mode_t::SHUTTING_DOWN, std::memory_order_release);

    // block main thread until all DECT NR+ connections were closed gracefully
    stop_request_block_nto();

    // close job queue so work functions will no longer be called
    job_queue.set_impermeable();

    // first stop accepting new data from application layer
    rd.application_server->stop_sc();

    // finally stop the data sink
    rd.application_client->stop_sc();
}

void tfw_p2p_rd_t::stop_request_block_nto() {
    std::unique_lock<std::mutex> lk(stop_mtx);

    while (!stop_released) {
        stop_cvar.wait(lk);
    }
}

void tfw_p2p_rd_t::stop_request_unblock() {
    std::unique_lock<std::mutex> lock(stop_mtx);
    stop_released = true;
    stop_cvar.notify_all();
}

}  // namespace dectnrp::upper::tfw::p2p
