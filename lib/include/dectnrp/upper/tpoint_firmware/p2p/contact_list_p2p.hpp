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

#include "dectnrp/mac/allocation/allocation_pt.hpp"
#include "dectnrp/mac/contact_list.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"

namespace dectnrp::upper::tfw::p2p {

class contact_list_p2p_t final : public mac::contact_list_t {
    public:
        // ##################################################
        // Radio Layer + PHY
        // -

        // ##################################################
        // MAC Layer

        /// allocation of each radio device class (um = unordered map)
        std::unordered_map<key_lrdid_t, section4::mac_architecture::identity_t> identity_pt_um;

        /// allocation of each radio device class (um = unordered map)
        std::unordered_map<key_lrdid_t, mac::allocation::allocation_pt_t> allocation_pt_um;

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer

        /// from app to lower layers
        common::adt::bimap_t<key_lrdid_t, uint32_t, true> app_server_idx;

        /// from lower layers to app
        common::adt::bimap_t<key_lrdid_t, uint32_t, true> app_client_idx;
};

}  // namespace dectnrp::upper::tfw::p2p
