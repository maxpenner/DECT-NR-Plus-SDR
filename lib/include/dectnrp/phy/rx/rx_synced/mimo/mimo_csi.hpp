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

#include "dectnrp/common/adt/expiring.hpp"
#include "dectnrp/phy/rx/rx_synced/mimo/mimo_report.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"
#include "dectnrp/sections_part4/physical_header_field/feedback_info.hpp"

namespace dectnrp::phy {

class mimo_csi_t {
    public:
        mimo_csi_t() = default;

        /**
         * \brief The current channel state information (CSI) can be updated based on the reporting
         * of the physical layer. This is a valid approach under the assumption that the channel is
         * reciprocal, which depends on the radio hardware in use. Once updated, the CSI may be
         * applied immediately.
         *
         * \param mimo_report
         * \param sync_report contains time of reception
         */
        void update(const mimo_report_t& mimo_report, const sync_report_t& sync_report);

        /**
         * \brief The CSI can also be updated based on the receiver's feedback as part of the PLCF.
         * In that case, the channel does not have to be reciprocal as the receiver sees the radio
         * hardware as part of its own channel. Once updated, the CSI may be applied immediately.
         *
         * \param feedback_format feedback format received
         * \param feedback_info_pool feedback pool with all feedback formats
         * \param sync_report contains time of reception and number of RX antennas
         */
        void update(const uint32_t feedback_format,
                    const section4::feedback_info_pool_t& feedback_info_pool,
                    const sync_report_t& sync_report);

        common::adt::expiring_t<uint32_t> MCS{};
        common::adt::expiring_t<uint32_t> codebook_index{};
        common::adt::expiring_t<uint32_t> tm_mode{};

    private:
        void update_mcs(const uint32_t MCS_, const int64_t time_64);
        void update_ci(const uint32_t codebook_index_, const int64_t time_64);
};

}  // namespace dectnrp::phy
