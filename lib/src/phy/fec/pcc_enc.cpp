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

#include "dectnrp/phy/fec/pcc_enc.hpp"

#include <cstring>

extern "C" {
#include "srsran/phy/fec/turbo/rm_turbo.h"
#include "srsran/phy/scrambling/scrambling.h"
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/sections_part3/fix/cbsegm.hpp"

static constexpr uint32_t crc_bits = 16;

static constexpr uint32_t SRSRAN_PDSCH_MAX_TDEC_ITERS = 5;

static constexpr uint16_t mask_none = 0x0000;
static constexpr uint16_t mask_mimo_cl = 0x5555;
static constexpr uint16_t mask_bf = 0xAAAA;
static constexpr uint16_t mask_mimo_cl_bf = 0xFFFF;

static constexpr uint32_t pcc_g_init = 0x44454354;

namespace dectnrp::phy {

int pcc_enc_init(pcc_enc_t* q) {
    int ret = SRSRAN_ERROR;

    // maximum length of PLCF
    const uint32_t plcf_bits_max = constants::plcf_type_2_bit;
    const uint32_t nof_bits_max_0 = plcf_bits_max + crc_bits;

    // after turbo encoding, we have 3 time the bits + the tails bits
    const uint32_t nof_bits_max_1 =
        common::adt::ceil_divide_integer(3U * nof_bits_max_0 + 3U * 4U, 8U) * 8U;

    dectnrp_assert(nof_bits_max_0 % 8 == 0, "Number of PLCF bits + CRC not a multiple of 8.");

    q->c_systematic = srsran_vec_u8_malloc(nof_bits_max_0 / 8);
    if (!q->c_systematic) {
        goto clean;
    }

    q->c_parity = srsran_vec_u8_malloc(nof_bits_max_1 / 8);
    if (!q->c_parity) {
        goto clean;
    }

    /* After ratematching we always have 196 bits (QPSK for 98 PCC subcarriers), however, input can
     * be either int8_t or int16_t, therefore *2. The data type determined by q->llr_bit_width.
     *
     * unpacked
     */
    q->e_rx = (void*)srsran_vec_u8_malloc(constants::pcc_bits * 2);
    if (!q->e_rx) {
        goto clean;
    }

    q->c_rx = srsran_vec_u8_malloc(nof_bits_max_0 / 8);
    if (!q->c_rx) {
        goto clean;
    }

    // crc of plcf is not 24 bits as for LTE, but only 16 bits, see 7.5.2.1
    if (srsran_crc_init(&q->crc_tb, SRSRAN_LTE_CRC16, crc_bits)) {
        goto clean;
    }

    // second argument is uint32_t max_long_cb, usually 6144
    if (srsran_tcod_init(&q->encoder, nof_bits_max_0)) {
        goto clean;
    }

    // second argument is uint32_t max_long_cb, usually 6144
    if (srsran_tdec_init(&q->decoder, nof_bits_max_0)) {
        goto clean;
    }
    // second argument is length of sequence used for scrambling, here to 196 bits with next
    // full byte being 200 (necessary if xor'ed with packed d bits), value 0x44454354 from 7.5.4
    if (srsran_sequence_LTE_pr(
            &q->seq, common::adt::ceil_divide_integer(constants::pcc_bits, 8U) * 8U, pcc_g_init)) {
        goto clean;
    }

    q->max_iterations = SRSRAN_PDSCH_MAX_TDEC_ITERS;

    q->llr_bit_width = PHY_LLR_BIT_WIDTH;

    srsran_rm_turbo_gentables();

    ret = SRSRAN_SUCCESS;

clean:
    if (ret == SRSRAN_ERROR) {
        pcc_enc_free(q);
        dectnrp_assert_failure("PCC initialization failed");
    }
    return ret;
}

int pcc_enc_free(pcc_enc_t* q) {
    if (q->c_systematic) {
        free(q->c_systematic);
    }
    if (q->c_parity) {
        free(q->c_parity);
    }
    if (q->e_rx) {
        free(q->e_rx);
    }
    if (q->c_rx) {
        free(q->c_rx);
    }
    srsran_rm_turbo_free_tables();
    srsran_tcod_free(&q->encoder);
    srsran_tdec_free(&q->decoder);
    srsran_sequence_free(&q->seq);
    return SRSRAN_SUCCESS;
}

void pcc_enc_encode(pcc_enc_t* q,
                    const uint8_t* const a,
                    uint8_t* d,
                    srsran_softbuffer_tx_t* softbuffer_d,
                    const uint32_t PLCF_type,
                    const bool cl,
                    const bool bf) {
    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "Unknown PLCF type");

    uint32_t N_PLCF_bits = 0;
    if (PLCF_type == 1) {
        N_PLCF_bits = constants::plcf_type_1_bit;
    } else {
        N_PLCF_bits = constants::plcf_type_2_bit;
    }

    uint32_t N_PLCF_byte = N_PLCF_bits / 8;

    // save local copy in q->c_systematic
    memcpy(q->c_systematic, a, N_PLCF_byte);

    // 7.5.2.4, attach crc
    srsran_crc_set_init(&q->crc_tb, 0);
    srsran_crc_attach_byte(&q->crc_tb, q->c_systematic, N_PLCF_bits);

    // 7.5.2.2 and 7.5.2.3, mask crc for closed loop MIMO or beamforming
    uint16_t mask;
    if (!cl && !bf) {
        mask = mask_none;
    } else if (cl && !bf) {
        mask = mask_mimo_cl;
    } else if (!cl && bf) {
        mask = mask_bf;
    } else {
        mask = mask_mimo_cl_bf;
    }

    uint16_t* tmp = (uint16_t*)&q->c_systematic[N_PLCF_byte];
    tmp[0] = tmp[0] ^ mask;

    // source: https://github.com/srsran/srsRAN/blob/master/lib/src/phy/fec/cbsegm.c
    // see at 6.1.4.2.3
    const uint32_t cblen_idx = sp3::fix::srsran_cbsegm_cbindex_FIX(N_PLCF_bits + crc_bits);

    srsran_tcod_encode_lut(&q->encoder,
                           &q->crc_tb,  // last argument is false, so crc_tb is ignored internally
                           NULL,        // no crc for codeblocks required
                           q->c_systematic,
                           q->c_parity,
                           cblen_idx,
                           false);  // we don't want srsran_tcod_encode_lut to attach the crc_tb, we
                                    // already did, so we pass is as not last cb

    if (srsran_rm_turbo_tx_lut(softbuffer_d->buffer_b[0],
                               q->c_systematic,
                               q->c_parity,
                               d,
                               cblen_idx,
                               constants::pcc_bits,  // PCC is always 196 bits
                               0,                    // no write offset
                               0))                   // rv always 0, see 7.5.3
    {
        dectnrp_assert_failure("Error in rate matching");
    }

    // scrambling
    // we want to scramble 196 bits, next number dividable by 8 is 200
    srsran_scrambling_bytes(&q->seq, d, 200);
}

bool pcc_enc_decode(pcc_enc_t* q,
                    uint8_t* a_rx,
                    const void* const d_rx,
                    srsran_softbuffer_rx_t* softbuffer_d_rx,
                    const uint32_t PLCF_type_test,
                    bool& cl,
                    bool& bf) {
    // we abort when we have found a correct crc
    bool early_stop = false;

    // convert type to length in bits and bytes
    const uint32_t N_PLCF_bits_blind_test =
        (PLCF_type_test == 1) ? constants::plcf_type_1_bit : constants::plcf_type_2_bit;
    const uint32_t N_PLCF_byte_blind_test = N_PLCF_bits_blind_test / 8;

    // source: https://github.com/srsran/srsRAN/blob/master/lib/src/phy/fec/cbsegm.c
    // see at 6.1.4.2.3
    const uint32_t cblen_idx =
        sp3::fix::srsran_cbsegm_cbindex_FIX(N_PLCF_bits_blind_test + crc_bits);

    if (q->llr_bit_width == 8) {
        // descramble
        memcpy(q->e_rx, d_rx, sizeof(int8_t) * constants::pcc_bits);
        srsran_scrambling_sb_offset(&q->seq, (int8_t*)q->e_rx, 0, constants::pcc_bits);

        // rate matching
        srsran_softbuffer_rx_reset(softbuffer_d_rx);
        if (srsran_rm_turbo_rx_lut_8bit((int8_t*)q->e_rx,
                                        (int8_t*)softbuffer_d_rx->buffer_f[0],
                                        constants::pcc_bits,
                                        cblen_idx,
                                        0)) {
            dectnrp_assert_failure("Error in rate matching");
        }

        // turbo decoding
        // srsran_tdec_force_not_sb(&q->decoder);
        srsran_tdec_new_cb(&q->decoder, N_PLCF_bits_blind_test + 16);
        // Run iterations and use CRC for early stopping
        uint32_t cb_noi = 0;
        do {
            srsran_tdec_iteration_8bit(&q->decoder, (int8_t*)softbuffer_d_rx->buffer_f[0], q->c_rx);

            ++cb_noi;

            // get checksum of decoded data
            uint16_t* c_rx_u16 = (uint16_t*)&q->c_rx[N_PLCF_byte_blind_test];
            uint16_t checksum_rx = c_rx_u16[0];

            // recalculate crc on decoded data
            srsran_crc_set_init(&q->crc_tb, 0);
            srsran_crc_attach_byte(&q->crc_tb, q->c_rx, N_PLCF_bits_blind_test);
            uint16_t checksum_rx_recalc = c_rx_u16[0];

            // check with masks
            if (!((checksum_rx ^ mask_none) - checksum_rx_recalc)) {
                cl = false;
                bf = false;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            } else if (!((checksum_rx ^ mask_mimo_cl) - checksum_rx_recalc)) {
                cl = true;
                bf = false;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            } else if (!((checksum_rx ^ mask_bf) - checksum_rx_recalc)) {
                cl = false;
                bf = true;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            } else if (!((checksum_rx ^ mask_mimo_cl_bf) - checksum_rx_recalc)) {
                cl = true;
                bf = true;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            }

        } while (cb_noi < q->max_iterations && !early_stop);

    } else if (q->llr_bit_width == 16) {
        // descrambling
        memcpy(q->e_rx, d_rx, sizeof(int16_t) * constants::pcc_bits);
        srsran_scrambling_s_offset(&q->seq, (int16_t*)q->e_rx, 0, constants::pcc_bits);

        // rate matching
        srsran_softbuffer_rx_reset(softbuffer_d_rx);
        if (srsran_rm_turbo_rx_lut((int16_t*)q->e_rx,
                                   (int16_t*)softbuffer_d_rx->buffer_f[0],
                                   constants::pcc_bits,
                                   cblen_idx,
                                   0)) {
            dectnrp_assert_failure("Error in rate matching");
        }

        // turbo decoding
        // srsran_tdec_force_not_sb(&q->decoder);
        srsran_tdec_new_cb(&q->decoder, N_PLCF_bits_blind_test + 16);
        // Run iterations and use CRC for early stopping
        uint32_t cb_noi = 0;
        do {
            srsran_tdec_iteration(&q->decoder, (int16_t*)softbuffer_d_rx->buffer_f[0], q->c_rx);

            ++cb_noi;

            // get checksum of decoded data
            uint16_t* c_rx_u16 = (uint16_t*)&q->c_rx[N_PLCF_byte_blind_test];
            uint16_t checksum_rx = c_rx_u16[0];

            // recalculate crc on decoded data
            srsran_crc_set_init(&q->crc_tb, 0);
            srsran_crc_attach_byte(&q->crc_tb, q->c_rx, N_PLCF_bits_blind_test);
            uint16_t checksum_rx_recalc = c_rx_u16[0];

            // check with masks
            if (!((checksum_rx ^ mask_none) - checksum_rx_recalc)) {
                cl = false;
                bf = false;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            } else if (!((checksum_rx ^ mask_mimo_cl) - checksum_rx_recalc)) {
                cl = true;
                bf = false;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            } else if (!((checksum_rx ^ mask_bf) - checksum_rx_recalc)) {
                cl = false;
                bf = true;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            } else if (!((checksum_rx ^ mask_mimo_cl_bf) - checksum_rx_recalc)) {
                cl = true;
                bf = true;
                softbuffer_d_rx->cb_crc[0] = true;
                early_stop = true;
            }

        } while (cb_noi < q->max_iterations && !early_stop);
    } else {
        dectnrp_assert_failure("Unknown value of llr_bit_width");
    }

    if (early_stop) {
        // save local copy in a_rx
        memcpy(a_rx, q->c_rx, N_PLCF_byte_blind_test);

        return true;
    }

    return false;
}

}  // namespace dectnrp::phy
