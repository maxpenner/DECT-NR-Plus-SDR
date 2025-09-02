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

#include "dectnrp/mac/contact_list.hpp"
#include "dectnrp/upper/p2p/data/contact_p2p.hpp"

// #define TFW_P2P_FT_ALIGN_BEACON_START_TO_FULL_SECOND_OR_CORRECT_OFFSET

namespace dectnrp::upper::tfw::p2p {

class ft_t {
    public:
        /// FTs use a fixed transmit power that must be written into the PLCF
        float TransmitPower_dBm;

        /// FTs use a variable RX power target that is announces to PTs in the beacon
        float ReceiverPower_dBm;

        /// fast lookup of all PTs and their properties
        mac::contact_list_t<contact_p2p_t> contact_list;
};

}  // namespace dectnrp::upper::tfw::p2p
