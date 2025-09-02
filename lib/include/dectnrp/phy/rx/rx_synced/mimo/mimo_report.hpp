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

namespace dectnrp::phy {

class mimo_report_t {
    public:
        mimo_report_t() = default;
        ~mimo_report_t() = default;

        /// own number of physical antennas
        uint32_t N_RX;

        /**
         * \brief Number of transmit streams N_TS, i.e. number of effective antennas N_eff_TX, as
         * received from the opposite site.
         */
        uint32_t N_TS_other{};

        /**
         * \brief These are the recommended beamforming matrix indices for MIMO modes with a single
         * spatial stream, namely mode 3 and 7. For transmit diversity modes, no beamforming matrix
         * is recommended, instead index 0 shall be used.
         */

        /// recommendation mode 3
        uint32_t tm_3_7_beamforming_idx{};
        uint32_t tm_3_7_beamforming_reciprocal_idx{};

        /**
         * \brief These are the recommended beamforming matrix indices for MIMO modes with more than
         * a single spatial stream.
         */

        uint32_t RI{};   // rank indicator
        uint32_t CQI{};  // channel quality indicator
        uint32_t PMI{};  // pre coding matrix indicator
};

}  // namespace dectnrp::phy
