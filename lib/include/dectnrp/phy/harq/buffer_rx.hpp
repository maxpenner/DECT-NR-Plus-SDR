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

#pragma once

extern "C" {
#include "srsran/phy/fec/softbuffer.h"
}

#include "dectnrp/phy/harq/buffer.hpp"

namespace dectnrp::phy::harq {

class buffer_rx_t final : public buffer_t {
    public:
        /// TB
        explicit buffer_rx_t(const uint32_t N_TB_byte_max,
                             const uint32_t G_max,
                             const uint32_t C_max,
                             const uint32_t Z);
        ~buffer_rx_t();

        buffer_rx_t() = delete;
        buffer_rx_t(const buffer_rx_t&) = delete;
        buffer_rx_t& operator=(const buffer_rx_t&) = delete;
        buffer_rx_t(buffer_rx_t&&) = delete;
        buffer_rx_t& operator=(buffer_rx_t&&) = delete;

        srsran_softbuffer_rx_t* get_softbuffer_d() { return &softbuffer_d; }

        void reset_a_cnt_and_softbuffer() override final;
        void reset_a_cnt_and_softbuffer(const uint32_t nof_cb) override final;

    private:
        srsran_softbuffer_rx_t softbuffer_d;
};

}  // namespace dectnrp::phy::harq
