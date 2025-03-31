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

#include "dectnrp/phy/harq/process_rx.hpp"

namespace dectnrp::phy::harq {

process_rx_t::process_rx_t(const uint32_t id_, const section3::packet_sizes_t maximum_packet_sizes)
    : process_t(id_) {
    hb_tb = std::make_unique<buffer_rx_t>(maximum_packet_sizes.N_TB_byte,
                                          maximum_packet_sizes.G,
                                          maximum_packet_sizes.C,
                                          maximum_packet_sizes.psdef.Z);

    hb_tb->reset_a_cnt_and_softbuffer(maximum_packet_sizes.C);
}

void process_rx_t::finalize(const bool crc_status) {
    switch (finalize_rx) {
        using enum finalize_rx_t;

        case reset_and_terminate:
            reset_and_terminate_func();
            break;

        case keep_running:
            unlock_inner();
            break;

        case keep_running_or_reset_and_terminate_if_crc_correct:
            if (crc_status) [[likely]] {
                reset_and_terminate_func();
            } else [[unlikely]] {
                unlock_inner();
            }
            break;
    }
}

void process_rx_t::reset_and_terminate_func() {
    // Note that resetting the softbuffer requires packet_sizes.C to be defined. So first reset
    // variables of the deriving class and ...
    hb_tb->reset_a_cnt_and_softbuffer(packet_sizes.C);
    finalize_rx = finalize_rx_t::reset_and_terminate;

    // ... then those of the base class.
    process_t::reset();
    process_t::unlock_inner();
    process_t::unlock_outer();
}

}  // namespace dectnrp::phy::harq
