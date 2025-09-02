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

#include <optional>
#include <vector>

#include "dectnrp/limits.hpp"
#include "dectnrp/phy/rx/chscan/chscan.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"
#include "dectnrp/phy/tx/tx_descriptor.hpp"

namespace dectnrp::phy {

class machigh_phy_tx_t {
    public:
        machigh_phy_tx_t() { tx_descriptor_vec.reserve(limits::max_nof_tx_packet_mac_to_phy); }

        typedef std::vector<tx_descriptor_t> tx_descriptor_vec_t;

        /**
         * \brief Vector of TX packets to generate. An instance of worker_tx_rx always starts
         * generating the packet defined by the first elements, then the second etc.
         */
        tx_descriptor_vec_t tx_descriptor_vec{};

        /**
         * \brief If the irregular report contains a finite time, the PHY will call work_irregular()
         * as soon as the time has passed.
         */
        irregular_report_t irregular_report{};
};

class machigh_phy_t final : public machigh_phy_tx_t {
    public:
        typedef std::optional<chscan_t> chscan_opt_t;

        /**
         * \brief After generating TX packets, the worker performs a channel estimation if not
         * empty.
         */
        chscan_opt_t chscan_opt{std::nullopt};
};

}  // namespace dectnrp::phy
