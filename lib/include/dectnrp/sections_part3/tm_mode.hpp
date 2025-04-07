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

namespace dectnrp::section3::tmmode {

using tm_mode_t = struct tm_mode_t {
        uint32_t index{};
        uint32_t N_eff_TX{};
        uint32_t N_SS{};
        bool cl{};

        /*
         * Do we need a bool for beamforming? No.
         *
         * Beamforming is always used and therefore true, when any matrix other than the identity
         * matrix is used (see 6.3.4 in part 3). This is equivalent to using any codebook index
         * other than 0.
         *
         * Beamforming is used at two points within the transmitter: 1) Naturally, it is used
         * between the transmit streams and the beamformed antenna streams. 2) The CRC of the PCC is
         * XOR masked when using beamforming, so that the receiver knows whether a packet can be
         * used for channel estimation.
         *
         * TX in tx.cpp needs to be given the codebook index to properly beamform, and fec needs to
         * knows whether the codebook index is larger than 0 (a.k.s. beamforming = true) so it can
         * determine which mask to use. So, instead of having a bool here (which would depend on the
         * codebook index in use), the only variable influencing beamforming is the codebook index
         * itself.
         */

        uint32_t N_TS{};
        uint32_t N_TX{};
};

tm_mode_t get_tm_mode(const uint32_t index);

/**
 * \brief See Table 7.2-1 in ETSI TS 103 636-3: The maximum transmission mode index for a given
 * number N_TX is always a N_TX x N_TX MIMO mode. This transmission mode also maximizes the amount
 * of data we can put in a single packet. This is important for determining the maximum packet
 * sizes.
 *
 * \param N_TX
 * \return
 */
uint32_t get_max_tm_mode_index_depending_on_N_TX(const uint32_t N_TX);

/// Table 7.2-1: TX diversity mode for a given number of antennas
uint32_t get_tx_div_mode(const uint32_t N_TX);

/// Table 7.2-1: single antenna mode
uint32_t get_single_antenna_mode(const uint32_t N_TX);

/// Table 7.2-1: Transmission modes and transmission mode signalling
uint32_t get_equivalent_tm_mode(const uint32_t N_eff_TX, const uint32_t N_SS);

}  // namespace dectnrp::section3::tmmode
