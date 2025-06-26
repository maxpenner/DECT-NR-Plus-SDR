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

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/upper/p2p/data/contact_p2p.hpp"

#define TFW_P2P_PT_AGC_MINIMUM_PERIOD_MS 48
#define TFW_P2P_PT_AGC_CHANGE_TIME_SAME_OR_DELAYED

namespace dectnrp::upper::tfw::p2p {

class pt_t {
    public:
        /// last time the AGC was tuned
        int64_t time_of_last_agc_change{common::adt::UNDEFINED_EARLY_64};

        /**
         * \brief The FT saves a full contact_list_t with one entry per PT. A single PT saves a
         * single contact with information about itself.
         */
        contact_p2p_t contact_pt;
};

}  // namespace dectnrp::upper::tfw::p2p
