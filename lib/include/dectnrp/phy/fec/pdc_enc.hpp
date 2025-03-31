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

#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"

namespace dectnrp::section3 {

using pdc_enc_t = struct pdc_enc_t {
        /// encoding intermediary steps
        uint8_t* c_systematic;
        uint8_t* c_parity;

        /// sub elements
        srsran_crc_t crc_cb;
        srsran_crc_t crc_tb;
        srsran_tcod_t encoder;
        srsran_tdec_t decoder;

        /// configuration
        uint32_t max_iterations;  // turbo decoder
        uint32_t llr_bit_width;   // width of input
};

int pdc_enc_init(pdc_enc_t* q, const packet_sizes_t& packet_sizes_maximum);

int pdc_enc_free(pdc_enc_t* q);

void pdc_encode_codeblocks(pdc_enc_t* q,
                           srsran_softbuffer_tx_t* softbuffer,
                           srsran_cbsegm_t* cb_segm,
                           const uint32_t Qm,
                           const uint32_t rv,
                           const uint32_t nof_e_bits,
                           const uint8_t* const data,
                           uint8_t* e_bits,

                           // newly added to allow codeblock-wise encoding
                           uint32_t& cb_idx,
                           uint32_t& rp,
                           uint32_t& wp,
                           const uint32_t nof_d_bits_minimum,
                           srsran_sequence_t* srsran_sequence);

bool pdc_decode_codeblocks(pdc_enc_t* q,
                           srsran_softbuffer_rx_t* softbuffer,
                           srsran_cbsegm_t* cb_segm,
                           uint32_t Qm,
                           uint32_t rv,
                           uint32_t nof_e_bits,
                           int16_t* e_bits,
                           uint8_t* data,

                           // newly added to allow codeblock-wise decoding
                           uint32_t& cb_idx,
                           uint32_t& wp,
                           const uint32_t nof_d_bits_maximum,
                           srsran_sequence_t* srsran_sequence);
}  // namespace dectnrp::section3
