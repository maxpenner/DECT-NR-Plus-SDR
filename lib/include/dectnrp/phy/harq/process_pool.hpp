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

#include <cstdint>
#include <memory>
#include <vector>

#include "dectnrp/phy/harq/process_rx.hpp"
#include "dectnrp/phy/harq/process_tx.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"

namespace dectnrp::phy::harq {

class process_pool_t {
    public:
        explicit process_pool_t(const sp3::packet_sizes_t maximum_packet_sizes,
                                const uint32_t nof_process_tx,
                                const uint32_t nof_process_rx);
        ~process_pool_t() = default;

        process_pool_t() = delete;
        process_pool_t(const process_pool_t&) = delete;
        process_pool_t& operator=(const process_pool_t&) = delete;
        process_pool_t(process_pool_t&&) = delete;
        process_pool_t& operator=(process_pool_t&&) = delete;

        process_tx_t* get_process_tx(const uint32_t PLCF_type,
                                     const uint32_t network_id,
                                     const sp3::packet_sizes_def_t psdef,
                                     const finalize_tx_t ftx) const;

        process_rx_t* get_process_rx(const uint32_t PLCF_type,
                                     const uint32_t network_id,
                                     const sp3::packet_sizes_def_t psdef,
                                     uint32_t rv,
                                     const finalize_rx_t frx) const;

        process_tx_t* get_process_tx_running(const uint32_t id, const finalize_tx_t ftx) const;

        process_rx_t* get_process_rx_running(const uint32_t id,
                                             uint32_t rv,
                                             const finalize_rx_t frx) const;

    private:
        std::vector<std::unique_ptr<process_tx_t>> hp_tx_vec;
        std::vector<std::unique_ptr<process_rx_t>> hp_rx_vec;
};

}  // namespace dectnrp::phy::harq
