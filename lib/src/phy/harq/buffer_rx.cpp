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

#include "dectnrp/phy/harq/buffer_rx.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/phy_config.hpp"

namespace dectnrp::phy::harq {

buffer_rx_t::buffer_rx_t(const uint32_t N_TB_byte_max,
                         const uint32_t G_max,
                         const uint32_t C_max,
                         const uint32_t Z)
    /* N_TB_byte_max and G_max are maximums of radio device class.
     *
     * a*: for TB at most N_TB_byte_max byte packed
     * d*: for PDC at most G bits UNpacked, *2 if int16_t
     *
     * softbuffer_d: unpacked of maximum size HARQ_SOFTBUFFER_SIZE_Z_2048_PDC or
     * HARQ_SOFTBUFFER_SIZE_Z_6144_PDC
     */
    : buffer_t(N_TB_byte_max, G_max * PHY_D_RX_DATA_TYPE_SIZE) {
    if (Z == 2048) {
        if (srsran_softbuffer_rx_init_guru(&softbuffer_d, C_max, HARQ_SOFTBUFFER_SIZE_Z_2048_PDC) !=
            SRSRAN_SUCCESS) {
            dectnrp_assert_failure("Softbuffer not initialized.");
        }
    } else {
        if (srsran_softbuffer_rx_init_guru(&softbuffer_d, C_max, HARQ_SOFTBUFFER_SIZE_Z_6144_PDC) !=
            SRSRAN_SUCCESS) {
            dectnrp_assert_failure("Softbuffer not initialized.");
        }
    }

    reset_a_cnt_and_softbuffer();
}

buffer_rx_t::~buffer_rx_t() { srsran_softbuffer_rx_free(&softbuffer_d); }

void buffer_rx_t::reset_a_cnt_and_softbuffer() {
    a_cnt = 0;
    srsran_softbuffer_rx_reset(&softbuffer_d);
}

void buffer_rx_t::reset_a_cnt_and_softbuffer(const uint32_t nof_cb) {
    a_cnt = 0;
    srsran_softbuffer_rx_reset_cb(&softbuffer_d, nof_cb);
}

}  // namespace dectnrp::phy::harq
