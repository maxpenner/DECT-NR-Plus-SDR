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

#include "dectnrp/phy/rx/rx_synced/pcc_report.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"

namespace dectnrp::phy {

class phy_maclow_t {
    public:
        explicit phy_maclow_t(const sync_report_t& sync_report_, const pcc_report_t& pcc_report_)
            : sync_report(sync_report_),
              pcc_report(pcc_report_){};

        const sync_report_t& sync_report;
        const pcc_report_t& pcc_report;
};

}  // namespace dectnrp::phy
