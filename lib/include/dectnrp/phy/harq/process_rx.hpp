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

#include <cstdint>
#include <memory>

#include "dectnrp/phy/harq/buffer_rx.hpp"
#include "dectnrp/phy/harq/finalize.hpp"
#include "dectnrp/phy/harq/process.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"

namespace dectnrp::phy::harq {

class process_rx_t final : public process_t {
    public:
        explicit process_rx_t(const uint32_t id_,
                              const section3::packet_sizes_t maximum_packet_sizes);
        ~process_rx_t() = default;

        process_rx_t() = delete;
        process_rx_t(const process_rx_t&) = delete;
        process_rx_t& operator=(const process_rx_t&) = delete;
        process_rx_t(process_rx_t&&) = delete;
        process_rx_t& operator=(process_rx_t&&) = delete;

        void finalize(const bool crc_status);

        buffer_rx_t* get_hb_tb() { return hb_tb.get(); };

        friend class process_pool_t;

    private:
        std::unique_ptr<buffer_rx_t> hb_tb;

        finalize_rx_t finalize_rx{finalize_rx_t::reset_and_terminate};

        void reset_and_terminate_func() override final;
};

}  // namespace dectnrp::phy::harq
