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
#include <unordered_map>

#include "dectnrp/common/adt/bimap.hpp"
#include "dectnrp/phy/rx/rx_synced/mimo/mimo_report.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"

namespace dectnrp::mac {

class contact_list_t {
    public:
        /// LRDID = long radio device ID
        using key_lrdid_t = uint32_t;

        // ##################################################
        // Radio Layer + PHY

        std::unordered_map<key_lrdid_t, phy::sync_report_t> sync_report_last_known;

        bool have_heard_from_at_or_after(const key_lrdid_t key_lrdid, const int64_t time_64) const;

        std::unordered_map<key_lrdid_t, phy::mimo_report_t> mimo_report_last_known;

        // ##################################################
        // MAC Layer

        /// long to short radio device mapping
        common::adt::bimap_t<key_lrdid_t, uint32_t, true> lrdid2srdid;

        bool is_lrdid_known(const key_lrdid_t key_lrdid) const;
        bool is_srdid_known(const uint32_t srdid) const;

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer
        // -
};

}  // namespace dectnrp::mac
