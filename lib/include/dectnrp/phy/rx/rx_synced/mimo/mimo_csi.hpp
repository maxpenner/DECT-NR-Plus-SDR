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

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/phy/rx/rx_synced/mimo/mimo_report.hpp"

namespace dectnrp::phy {

class mimo_csi_t {
    public:
        mimo_csi_t() = default;

        template <typename T>
        class expiring_t {
                int64_t time_64;
                T val{};
        };

        /// provided by MAC through a received PLCF
        void update(const uint32_t MCS_);

        /// provided by PHY
        void update(const mimo_report_t& mimo_report);

        expiring_t<uint32_t> MCS{};
        expiring_t<uint32_t> tm_mode{};
        expiring_t<uint32_t> codebook_index{};
};

}  // namespace dectnrp::phy
