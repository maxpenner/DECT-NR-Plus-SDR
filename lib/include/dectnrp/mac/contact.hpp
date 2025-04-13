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
#include <vector>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/mac/allocation/allocation_pt.hpp"
#include "dectnrp/phy/rx/rx_synced/mimo/mimo_csi.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"

namespace dectnrp::mac {

class contact_t {
    public:
        // ##################################################
        // Radio Layer + PHY

        phy::sync_report_t sync_report;

        // ##################################################
        // MAC Layer

        section4::mac_architecture::identity_t identity;
        mac::allocation::allocation_pt_t allocation_pt;
        phy::mimo_csi_t mimo_csi;

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer

        uint32_t conn_idx_server{common::adt::UNDEFINED_NUMERIC_32};
        uint32_t conn_idx_client{common::adt::UNDEFINED_NUMERIC_32};
};

}  // namespace dectnrp::mac
