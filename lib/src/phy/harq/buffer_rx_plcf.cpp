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

#include "dectnrp/phy/harq/buffer_rx_plcf.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/phy_config.hpp"

namespace dectnrp::phy::harq {

buffer_rx_plcf_t::buffer_rx_plcf_t()
    /* a*: for PLCF type 1 or 2 packed
     * d*: for PCC always 196 bits unpacked, *2 if int16_t
     *
     * softbuffer_d_x0: unpacked of maximum size HARQ_SOFTBUFFER_SIZE_PCC
     */
    : buffer_t(constants::plcf_type_2_byte, constants::pcc_bits * PHY_D_RX_DATA_TYPE_SIZE) {
    // assume one codeblock
    if (srsran_softbuffer_rx_init_guru(&softbuffer_d_type_1, 1, HARQ_SOFTBUFFER_SIZE_PCC) !=
        SRSRAN_SUCCESS) {
        dectnrp_assert_failure("Softbuffer for PLCF type 1 not initialized.");
    }
    if (srsran_softbuffer_rx_init_guru(&softbuffer_d_type_2, 1, HARQ_SOFTBUFFER_SIZE_PCC) !=
        SRSRAN_SUCCESS) {
        dectnrp_assert_failure("Softbuffer for PLCF type 2 not initialized.");
    }

    reset_a_cnt_and_softbuffer();
}

buffer_rx_plcf_t::~buffer_rx_plcf_t() {
    srsran_softbuffer_rx_free(&softbuffer_d_type_1);
    srsran_softbuffer_rx_free(&softbuffer_d_type_2);
}

void buffer_rx_plcf_t::reset_a_cnt_and_softbuffer() {
    a_cnt = 0;
    srsran_softbuffer_rx_reset(&softbuffer_d_type_1);
    srsran_softbuffer_rx_reset(&softbuffer_d_type_2);
}

void buffer_rx_plcf_t::reset_a_cnt_and_softbuffer([[maybe_unused]] const uint32_t nof_cb) {
    // we only have one code block
    a_cnt = 0;
    srsran_softbuffer_rx_reset_cb_crc(&softbuffer_d_type_1, 1);
    srsran_softbuffer_rx_reset_cb_crc(&softbuffer_d_type_2, 1);
}

}  // namespace dectnrp::phy::harq
