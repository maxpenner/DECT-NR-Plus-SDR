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

#include "dectnrp/upper/p2p/data/contact_p2p.hpp"

#define TFW_P2P_PT_AGC_ENABLED
#define TFW_P2P_PT_AGC_CHANGE_TIMED_OR_IMMEDIATE_PT

namespace dectnrp::upper::tfw::p2p {

struct pt_t {
        // ##################################################
        // Radio Layer + PHY
        // -

        // ##################################################
        // MAC Layer

        /**
         * \brief The FT saves a full contact_list_t with one entry per PT. A PT saves a single
         * contact with information about itself.
         */
        contact_p2p_t contact_pt;

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer
        // -

        // ##################################################
        // logging
        // -
};

}  // namespace dectnrp::upper::tfw::p2p
