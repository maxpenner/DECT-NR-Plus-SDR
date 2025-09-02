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

#pragma once

#include <memory>

extern "C" {
#include "srsran/phy/fec/softbuffer.h"
}

#include "dectnrp/phy/harq/buffer.hpp"

namespace dectnrp::phy::harq {

class buffer_rx_plcf_t final : public buffer_t {
    public:
        [[nodiscard]] static std::unique_ptr<buffer_rx_plcf_t> new_unique_instance() {
            return std::unique_ptr<buffer_rx_plcf_t>(new buffer_rx_plcf_t());
        }

        ~buffer_rx_plcf_t();

        buffer_rx_plcf_t(const buffer_rx_plcf_t&) = delete;
        buffer_rx_plcf_t& operator=(const buffer_rx_plcf_t&) = delete;
        buffer_rx_plcf_t(buffer_rx_plcf_t&&) = delete;
        buffer_rx_plcf_t& operator=(buffer_rx_plcf_t&&) = delete;

        srsran_softbuffer_rx_t* get_softbuffer_d_type_1() { return &softbuffer_d_type_1; }
        srsran_softbuffer_rx_t* get_softbuffer_d_type_2() { return &softbuffer_d_type_2; }

        void reset_a_cnt_and_softbuffer() override final;
        void reset_a_cnt_and_softbuffer(const uint32_t nof_cb) override final;

    private:
        buffer_rx_plcf_t();
        srsran_softbuffer_rx_t softbuffer_d_type_1;
        srsran_softbuffer_rx_t softbuffer_d_type_2;
};

}  // namespace dectnrp::phy::harq
