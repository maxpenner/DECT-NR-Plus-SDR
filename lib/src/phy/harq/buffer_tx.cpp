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

#include "dectnrp/phy/harq/buffer_tx.hpp"

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"

namespace dectnrp::phy::harq {

buffer_tx_t::buffer_tx_t(const COMPONENT_T component)
    /* a*: for PLCF type 1 or 2 packed
     * d*: for PCC always 196 bits packed (196+4=200 is next number divisable by 8)
     *
     * softbuffer_d: unpacked of maximum size HARQ_SOFTBUFFER_SIZE_PCC
     */
    : buffer_t(constants::plcf_type_2_byte,
               common::adt::ceil_divide_integer(constants::pcc_bits, 8U)) {
    dectnrp_assert(component == COMPONENT_T::PLCF, "Must be PLCF.");

    // assume one codeblock
    if (srsran_softbuffer_tx_init_guru(&softbuffer_d, 1, HARQ_SOFTBUFFER_SIZE_PCC) !=
        SRSRAN_SUCCESS) {
        dectnrp_assert_failure("Softbuffer not initialized.");
    }

    reset_a_cnt_and_softbuffer();
}

buffer_tx_t::buffer_tx_t(const COMPONENT_T component,
                         const uint32_t N_TB_byte_max,
                         const uint32_t G_max,
                         const uint32_t C_max,
                         const uint32_t Z)
    /* N_TB_byte_max and G_max are maximums of radio device class.
     *
     * a*: for TB at most N_TB_byte_max byte packed
     * d*: for PDC at most common::ceil_divide_integer<uint32_t>(G_max, 8) byte packed
     *
     * softbuffer_d: unpacked of maximum size HARQ_SOFTBUFFER_SIZE_Z_2048_PDC or
     * HARQ_SOFTBUFFER_SIZE_Z_6144_PDC
     */
    : buffer_t(N_TB_byte_max, common::adt::ceil_divide_integer(G_max, 8U)) {
    dectnrp_assert(component == COMPONENT_T::TRANSPORT_BLOCK, "Must be PLCF.");

    if (Z == 2048) {
        if (srsran_softbuffer_tx_init_guru(&softbuffer_d, C_max, HARQ_SOFTBUFFER_SIZE_Z_2048_PDC) !=
            SRSRAN_SUCCESS) {
            dectnrp_assert_failure("Softbuffer not initialized.");
        }
    } else {
        if (srsran_softbuffer_tx_init_guru(&softbuffer_d, C_max, HARQ_SOFTBUFFER_SIZE_Z_6144_PDC) !=
            SRSRAN_SUCCESS) {
            dectnrp_assert_failure("Softbuffer not initialized.");
        }
    }

    reset_a_cnt_and_softbuffer();
}

buffer_tx_t::~buffer_tx_t() { srsran_softbuffer_tx_free(&softbuffer_d); }

void buffer_tx_t::reset_a_cnt_and_softbuffer() {
    a_cnt = 0;
    srsran_softbuffer_tx_reset(&softbuffer_d);
}

void buffer_tx_t::reset_a_cnt_and_softbuffer(const uint32_t nof_cb) {
    a_cnt = 0;
    srsran_softbuffer_tx_reset_cb(&softbuffer_d, nof_cb);
}

}  // namespace dectnrp::phy::harq
