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

#include "dectnrp/phy/interfaces/maclow_phy.hpp"
#include "dectnrp/phy/interfaces/phy_maclow.hpp"
#include "dectnrp/phy/rx/rx_synced/pdc_report.hpp"

namespace dectnrp::phy {

class phy_machigh_t {
    public:
        explicit phy_machigh_t(const phy_maclow_t& phy_maclow_,
                               const maclow_phy_t& maclow_phy_,
                               const pdc_report_t& pdc_report_)
            : phy_maclow(phy_maclow_),
              maclow_phy(maclow_phy_),
              pdc_report(pdc_report_) {};

        const phy_maclow_t& phy_maclow;
        const maclow_phy_t& maclow_phy;
        const pdc_report_t& pdc_report;
};

}  // namespace dectnrp::phy
