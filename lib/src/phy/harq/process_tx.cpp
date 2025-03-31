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

#include "dectnrp/phy/harq/process_tx.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

#define ASSERT_FULLY_LOCKED dectnrp_assert(is_outer_locked_inner_locked(), "incorrect lock state")

namespace dectnrp::phy::harq {
process_tx_t::process_tx_t(const uint32_t id_, const section3::packet_sizes_t maximum_packet_sizes)
    : process_t(id_) {
    hb_plcf = std::make_unique<buffer_tx_t>(buffer_tx_t::COMPONENT_T::PLCF);

    hb_tb = std::make_unique<buffer_tx_t>(buffer_tx_t::COMPONENT_T::TRANSPORT_BLOCK,
                                          maximum_packet_sizes.N_TB_byte,
                                          maximum_packet_sizes.G,
                                          maximum_packet_sizes.C,
                                          maximum_packet_sizes.psdef.Z);

    hb_plcf->reset_a_cnt_and_softbuffer();
    hb_tb->reset_a_cnt_and_softbuffer(maximum_packet_sizes.C);
}

void process_tx_t::finalize() {
    switch (finalize_tx) {
        using enum finalize_tx_t;

        case reset_and_terminate:
            reset_and_terminate_func();
            break;

        case increase_rv_and_keep_running:
            set_next_rv();
            unlock_inner();
            break;

        case keep_rv_and_keep_running:
            unlock_inner();
            break;
    }
}

void process_tx_t::set_next_rv() {
    ASSERT_FULLY_LOCKED;

    switch (rv) {
        case 0:
            rv = 2;
            break;
        case 1:
            rv = 0;
            break;
        case 2:
            rv = 3;
            break;
        case 3:
            rv = 1;
            break;

        default:
            dectnrp_assert(false, "rv undefined");
            break;
    }

    ++rv_unwrapped;

    dectnrp_assert(rv_unwrapped <= constants::rv_unwrapped_max, "too many retransmission");
}

void process_tx_t::reset_and_terminate_func() {
    // Note that resetting the softbuffer requires packet_sizes.C to be defined. So first reset
    // variables of the deriving class and ...
    hb_plcf->reset_a_cnt_and_softbuffer();
    hb_tb->reset_a_cnt_and_softbuffer(packet_sizes.C);
    rv_unwrapped = 0;
    finalize_tx = finalize_tx_t::reset_and_terminate;

    // ... then those of the base class.
    process_t::reset();
    process_t::unlock_inner();
    process_t::unlock_outer();
}

}  // namespace dectnrp::phy::harq
