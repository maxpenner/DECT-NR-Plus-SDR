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

#include <cstdint>

namespace dectnrp::sp3 {

/**
 * \brief From a higher layer we receive the PLCF and we create the PCC. From a higher layer we
 * receive the TB and we create the PDC. These configuration variables are required for fec, and
 * they can be reused across multiple packets.
 */
struct fec_cfg_t {
        uint32_t PLCF_type = 0;    // PLCF, 1 for Type 1 and 2 for Type 2
        bool closed_loop = false;  // PLCF
        bool beamforming = false;  // PLCF
        uint32_t N_TB_bits = 0;    // TB
        uint32_t N_bps = 0;        // TB
        uint32_t rv = 0;           // TB
        uint32_t G = 0;            // TB, G = N_SS * N_PDC_subc * N_bps, see 7.6.6
        uint32_t network_id = 0;   // TB
        uint32_t Z = 0;            // TB
};

}  // namespace dectnrp::sp3
