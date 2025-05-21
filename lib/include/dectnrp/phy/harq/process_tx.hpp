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

#include "dectnrp/phy/harq/buffer_tx.hpp"
#include "dectnrp/phy/harq/finalize.hpp"
#include "dectnrp/phy/harq/process.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"

namespace dectnrp::phy::harq {

class process_tx_t final : public process_t {
    public:
        explicit process_tx_t(const uint32_t id_, const sp3::packet_sizes_t maximum_packet_sizes);
        ~process_tx_t() = default;

        process_tx_t() = delete;
        process_tx_t(const process_tx_t&) = delete;
        process_tx_t& operator=(const process_tx_t&) = delete;
        process_tx_t(process_tx_t&&) = delete;
        process_tx_t& operator=(process_tx_t&&) = delete;

        void finalize();

        buffer_tx_t* get_hb_plcf() { return hb_plcf.get(); };
        buffer_tx_t* get_hb_tb() { return hb_tb.get(); };

        uint8_t* get_a_plcf() { return hb_plcf->get_a(); };
        uint8_t* get_a_tb() { return hb_tb->get_a(); };

        friend class process_pool_t;

    private:
        std::unique_ptr<buffer_tx_t> hb_plcf;
        std::unique_ptr<buffer_tx_t> hb_tb;

        void set_next_rv();

        uint32_t rv_unwrapped{0};

        finalize_tx_t finalize_tx{finalize_tx_t::reset_and_terminate};

        void reset_and_terminate_func() override final;
};

}  // namespace dectnrp::phy::harq
