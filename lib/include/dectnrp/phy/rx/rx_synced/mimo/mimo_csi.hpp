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
         * \brief The CSI can be updated based on the receiver's feedback as part of the PLCF. In
         * that case, the channel does not have to be reciprocal as the receiver sees the radio
         * hardware as part of its own channel. Once updated, the CSI may be applied immediately.
         *
         * \param feedback_format feedback format received
         * \param feedback_info_pool feedback pool with all feedback formats
         * \param sync_report contains time of reception and number of RX antennas
         */
        void update_from_feedback(const uint32_t feedback_format,
                                  const sp4::feedback_info_pool_t& feedback_info_pool,
                                  const sync_report_t& sync_report);

        /**
         * \brief The current channel state information (CSI) can also be updated based on the
         * reporting of the physical layer. This is a valid approach under the assumption that the
         * channel is reciprocal, which depends on the radio hardware in use. Once updated, the CSI
         * may be applied immediately.
         *
         * \param mimo_report
         * \param sync_report contains time of reception
         */
        void update_from_phy(const mimo_report_t& mimo_report, const sync_report_t& sync_report);
        void update_from_phy(const uint32_t MCS, const sync_report_t& sync_report);

        common::adt::expiring_t<uint32_t> feedback_MCS{};
        common::adt::expiring_t<uint32_t> feedback_codebook_index{};
        common::adt::expiring_t<uint32_t> feedback_tm_mode{};

        common::adt::expiring_t<uint32_t> phy_MCS{};
        common::adt::expiring_t<uint32_t> phy_codebook_index{};
        common::adt::expiring_t<uint32_t> phy_codebook_index_reciprocal{};
        common::adt::expiring_t<uint32_t> phy_tm_mode_reciprocal{};

        /// can be the same as feedback_codebook_index or phy_codebook_index_reciprocal
        common::adt::expiring_t<uint32_t> codebook_index{};

        /// can be the same as feedback_tm_mode or phy_tm_mode_reciprocal
        common::adt::expiring_t<uint32_t> tm_mode{};
};

}  // namespace dectnrp::phy
