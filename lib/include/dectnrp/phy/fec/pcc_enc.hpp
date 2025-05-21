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

extern "C" {
#include "srsran/phy/common/sequence.h"
#include "srsran/phy/fec/crc.h"
#include "srsran/phy/fec/softbuffer.h"
#include "srsran/phy/fec/turbo/turbocoder.h"
#include "srsran/phy/fec/turbo/turbodecoder.h"
}

namespace dectnrp::phy {

struct pcc_enc_t {
        /// encoding intermediary steps
        uint8_t* c_systematic;
        uint8_t* c_parity;

        /// decoding intermediary steps
        void* e_rx;
        uint8_t* c_rx;

        /// sub elements
        srsran_crc_t crc_tb;  // required as input for internal function, but not used
        srsran_tcod_t encoder;
        srsran_tdec_t decoder;
        srsran_sequence_t seq = {};  // scrambling

        /// configuration
        uint32_t max_iterations;  // turbo decoder
        uint32_t llr_bit_width;   // width of "void* e_rx"
};

int pcc_enc_init(pcc_enc_t* q);

int pcc_enc_free(pcc_enc_t* q);

void pcc_enc_encode(pcc_enc_t* q,
                    const uint8_t* const a,
                    uint8_t* d,
                    srsran_softbuffer_tx_t* softbuffer_d,
                    const uint32_t PLCF_type,
                    const bool cl,
                    const bool bf);

bool pcc_enc_decode(pcc_enc_t* q,
                    uint8_t* a_rx,
                    const void* const d_rx,
                    srsran_softbuffer_rx_t* softbuffer_d_rx,
                    const uint32_t PLCF_type_test,
                    bool& cl,
                    bool& bf);

}  // namespace dectnrp::phy
