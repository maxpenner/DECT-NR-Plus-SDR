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
#include <optional>

#include "dectnrp/common/ant.hpp"

namespace dectnrp::radio {

struct buffer_tx_meta_t {
        /// first tx packet has order 0
        int64_t tx_order_id{-1};

        /// when set to a value >= 0, the next expected tx_order_id is overwritten in tx thread
        int64_t tx_order_id_expect_next{-1};

        /// transmission time of first sample
        int64_t tx_time_64{-1};

        /// adjustments applied after packet transmission
        std::optional<common::ant_t> tx_power_adj_dB{std::nullopt};
        std::optional<common::ant_t> rx_power_adj_dB{std::nullopt};

        /**
         * \brief At the end of a transmission, we can busy wait for the next transmission to
         * avoid unnecessary TX/RX switches. This usually only makes sense if the MAC layer
         * already knows that another packet will follow, but which might take some time to
         * fully calculate. If set to 0, busy-waiting is omitted.
         */
        uint32_t busy_wait_us{0};
};

}  // namespace dectnrp::radio
