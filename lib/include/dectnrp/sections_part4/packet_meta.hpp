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

#include "dectnrp/sections_part3/derivative/packet_sizes_def.hpp"
#include "dectnrp/sections_part3/derivative/tx_meta.hpp"

namespace dectnrp::section4 {

class packet_meta_t {
    public:
        /// defines all sizes of the transmission (bits, bytes and IQ samples)
        section3::packet_sizes_def_t psdef;

        /// defines additional meta data required on PHY, strictly speaking not DECT NR+ related
        section3::tx_meta_t tx_meta;
};

}  // namespace dectnrp::section4
