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

#include "dectnrp/phy/fec/pdc_enc.hpp"

#include <cstring>

extern "C" {
#include "srsran/phy/fec/turbo/rm_turbo.h"
#include "srsran/phy/scrambling/scrambling.h"
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/sections_part3/fix/cbsegm.hpp"
#include "dectnrp/sections_part3/fix/mod.hpp"
#include "dectnrp/sections_part3/fix/scrambling.hpp"

static constexpr uint32_t SRSRAN_PDSCH_MIN_TDEC_ITERS = 2;
static constexpr uint32_t SRSRAN_PDSCH_MAX_TDEC_ITERS = 10;

namespace dectnrp::section3 {

int pdc_enc_init(pdc_enc_t* q, const section3::packet_sizes_t& packet_sizes_maximum) {
    int ret = SRSRAN_ERROR;

    // we have up to Z = 2048 or 6144 bits per cb + 4 bit tail
    // packed
    q->c_systematic = srsran_vec_u8_malloc((packet_sizes_maximum.psdef.Z + 8) / 8);
    if (!q->c_systematic) {
        goto clean;
    }
    // after turbo coding we have up to 3*6144 + 3*4 = 18444 bits
    // packed
    q->c_parity = srsran_vec_u8_malloc((3 * packet_sizes_maximum.psdef.Z + 16) / 8);
    if (!q->c_parity) {
        goto clean;
    }

    // 6.1.2 and 7.6.2
    if (srsran_crc_init(&q->crc_cb, SRSRAN_LTE_CRC24B, 24)) {
        goto clean;
    }
    // 6.1.2 and 7.6.2
    if (srsran_crc_init(&q->crc_tb, SRSRAN_LTE_CRC24A, 24)) {
        goto clean;
    }
    // second argument is uint32_t max_long_cb, usually 6144 for LTE
    if (srsran_tcod_init(&q->encoder, packet_sizes_maximum.psdef.Z)) {
        goto clean;
    }
    // second argument is uint32_t max_long_cb, usually 6144 for LTE
    if (srsran_tdec_init(&q->decoder, packet_sizes_maximum.psdef.Z)) {
        goto clean;
    }

    q->max_iterations = SRSRAN_PDSCH_MAX_TDEC_ITERS;

    q->llr_bit_width = PHY_LLR_BIT_WIDTH;

    srsran_rm_turbo_gentables();

    srsran_tdec_force_not_sb(&q->decoder);

    ret = SRSRAN_SUCCESS;

clean:
    if (ret == SRSRAN_ERROR) {
        pdc_enc_free(q);
        dectnrp_assert_failure("PDC initialization failed");
    }
    return ret;
}

int pdc_enc_free(pdc_enc_t* q) {
    if (q->c_systematic) {
        free(q->c_systematic);
    }
    if (q->c_parity) {
        free(q->c_parity);
    }
    srsran_rm_turbo_free_tables();
    srsran_tcod_free(&q->encoder);
    srsran_tdec_free(&q->decoder);
    return SRSRAN_SUCCESS;
}

/* Template is function encode_tb_off() from
 * https://github.com/srsran/srsRAN/blob/master/lib/src/phy/phch/sch.c
 *
 * Changes:
 *
 *  0) When called with rv > 0, turbo encoding is not rerun. Instead, the systematic and parity bits
 * from the softbuffer called with rv=0 are reused for rate matching. This requires that for a
 * freshly reset softbuffer, this function must be called with rv=0 first before any other rv.
 *
 *  1) Instead of encoding all codeblocks of a transport block in a single step, this function only
 * encodes codeblocks until nof_d_bits_minimum-many bits are written. For this, there are external
 * counters for the current codeblock index cb_idx, as well as a read pointer rp and a write pointer
 * wp for the number of bits read or written. This approach greatly helps reducing the initial
 * transmission latency when having very large transport blocks.
 *
 *  2) Each codeblock is scrambled after rate mathing. The sequence is an input parameter
 * srsran_sequence_t* and the offset depends on rp and wp.
 *
 *  3) ToDo: The size of the soft buffer (n_soft_bits) influences the redundancy, see part
 * 3, 6.1.5.3 Bit collection, selection and transmission.
 *
 *  4) Moved all asserts to the beginning of the function.
 */
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
                           srsran_sequence_t* srsran_sequence) {
    dectnrp_assert(!(q == NULL || e_bits == NULL || cb_segm == NULL || softbuffer == NULL),
                   "Invalid parameters");
    dectnrp_assert(cb_segm->F == 0, "Filler bits not supported");
    dectnrp_assert(cb_segm->C <= softbuffer->max_cb, "Number of CBs exceeds maximum");
    dectnrp_assert(Qm != 0, "Invalid Qm");

    uint32_t Gp = nof_e_bits / Qm;

    uint32_t gamma = Gp;
    if (cb_segm->C > 0) {
        gamma = Gp % cb_segm->C;
    }

    uint32_t cblen_idx, cb_len, rlen, n_e;

    while (wp < nof_d_bits_minimum) {
        /* Get read lengths */
        if (cb_idx < cb_segm->C2) {
            cb_len = cb_segm->K2;
            cblen_idx = cb_segm->K2_idx;
        } else {
            cb_len = cb_segm->K1;
            cblen_idx = cb_segm->K1_idx;
        }
        if (cb_segm->C > 1) {
            rlen = cb_len - 24;
        } else {
            rlen = cb_len;
        }
        if (cb_idx <= cb_segm->C - gamma - 1) {
            n_e = Qm * (Gp / cb_segm->C);
        } else {
            n_e = Qm * ((uint32_t)ceilf((float)Gp / cb_segm->C));
        }

        if (rv == 0) {
            if (data) {
                bool last_cb = false;

                /* Copy data to another buffer, making space for the Codeblock CRC */
                if (cb_idx < cb_segm->C - 1) {
                    // Copy data
                    memcpy(q->c_systematic, &data[rp / 8], rlen * sizeof(uint8_t) / 8);
                } else {
                    /* Append Transport Block parity bits to the last CB */
                    memcpy(q->c_systematic, &data[rp / 8], (rlen - 24) * sizeof(uint8_t) / 8);
                    last_cb = true;
                }

                /* Turbo Encoding
                 * If Codeblock CRC is required it is given the CRC instance pointer, otherwise CRC
                 * pointer shall be NULL
                 */
                srsran_tcod_encode_lut(&q->encoder,
                                       &q->crc_tb,
                                       (cb_segm->C > 1) ? &q->crc_cb : NULL,
                                       q->c_systematic,  // q->cb_in,
                                       q->c_parity,      // q->parity_bits,
                                       cblen_idx,
                                       last_cb);
            } else {
                dectnrp_assert_failure("Pointer data not valid");
            }
        }

        /* Rate matching */
        if (srsran_rm_turbo_tx_lut(softbuffer->buffer_b[cb_idx],
                                   q->c_systematic,
                                   q->c_parity,
                                   &e_bits[wp / 8],
                                   cblen_idx,
                                   n_e,
                                   wp % 8,
                                   rv)) {
            dectnrp_assert_failure("Error in rate matching");
        }

        // scrambling
        section3::fix::srsran_scrambling_bytes_with_sequence_offset_FIX(
            srsran_sequence, e_bits, n_e, wp);

        /* Set read/write pointers */
        rp += rlen;
        wp += n_e;

        ++cb_idx;
    }
}

/* Template is function decode_tb_cb() from
 * https://github.com/srsran/srsRAN/blob/master/lib/src/phy/phch/sch.c
 *
 * Changes:
 *
 *  0) Insert (int8_t*) and (int16_t*).
 *
 *  1) Instead of starting with C1 (=C+) and K1, we start with C2 (=C_) and K2.
 * https://github.com/srsran/srsRAN/blob/master/lib/src/phy/fec/cbsegm.c
 *
 *      uint32_t cb_len     = cb_idx < cb_segm->C2 ? cb_segm->K2 : cb_segm->K1;
 *      uint32_t cb_len_idx = cb_idx < cb_segm->C2 ? cb_segm->K2_idx : cb_segm->K1_idx;
 *
 *  2) Instead of using srsran_rm_turbo_rx_lut, we use srsran_rm_turbo_rx_lut_ with
 * enable_input_tdec=false.
 * https://github.com/srsran/srsRAN/blob/master/lib/src/phy/fec/turbo/rm_turbo.c
 *
 *      srsran_rm_turbo_rx_lut_(&e_bits_s[rp], softbuffer->buffer_f[cb_idx], n_e2, cb_len_idx, rv,
 * false);
 *
 *  3) Remove references to avg_iterations.
 *
 *  4) Pull the following lines into the loop over cb_idx.
 *
 *      uint32_t cb_len     = cb_idx < cb_segm->C2 ? cb_segm->K2 : cb_segm->K1;
 *      uint32_t cb_len_idx = cb_idx < cb_segm->C2 ? cb_segm->K2_idx : cb_segm->K1_idx;
 *      uint32_t rlen  = cb_segm->C == 1 ? cb_len : (cb_len - 24);
 *
 *  5) Insert write_offset and replace cb_idx * rlen.
 *
 *      uint32_t write_offset = 0;
 *
 *  6) For 8 bit decoding.
 *
 *      see phy_config.h
 *
 *  7) Change ERROR to ERROR DECT.
 *
 *  8) Deactive loop over if (!softbuffer->cb_crc[cb_idx]) { and also the else case.
 *
 *  9) Some signedness errors.
 *
 *  10) Add definitions:
 *
 *      SRSRAN_PDSCH_MIN_TDEC_ITERS = 2
 *      SRSRAN_PDSCH_MAX_TDEC_ITERS = 10
 *
 *  11) Comment out
 *
 *          if (!softbuffer->cb_crc[cb_idx]) {
 *
 *      and the else case.
 *
 *  12) Disable the final if case, i.e.
 *
 *          if (!softbuffer->tb_crc)
 *
 *      completely. This way each new rv for each CB is processed, even if the checksum was correct
 * in a previous CB.
 */
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
                           srsran_sequence_t* srsran_sequence) {
    dectnrp_assert(!(q == NULL || e_bits == NULL || cb_segm == NULL || softbuffer == NULL),
                   "Invalid parameters");
    dectnrp_assert(cb_segm->F == 0, "Filler bits not supported");
    dectnrp_assert(cb_segm->C <= softbuffer->max_cb, "Number of CBs exceeds maximum");
    dectnrp_assert(cb_segm->C <= SRSRAN_MAX_CODEBLOCKS, "Number of CBs exceeds maximum");
    dectnrp_assert(Qm != 0, "Invalid Qm");
    dectnrp_assert(data != NULL, "Invalid data pointer.");

    int8_t* e_bits_b = (int8_t*)e_bits;
    int16_t* e_bits_s = (int16_t*)e_bits;

    while (cb_idx < cb_segm->C) {
        const uint32_t cb_len = cb_idx < cb_segm->C2 ? cb_segm->K2 : cb_segm->K1;
        const uint32_t cb_len_idx = cb_idx < cb_segm->C2 ? cb_segm->K2_idx : cb_segm->K1_idx;

        const uint32_t rlen = cb_segm->C == 1 ? cb_len : (cb_len - 24);

        const uint32_t Gp = nof_e_bits / Qm;
        const uint32_t gamma = cb_segm->C > 0 ? Gp % cb_segm->C : Gp;
        const uint32_t n_e = Qm * (Gp / cb_segm->C);

        uint32_t rp = cb_idx * n_e;
        uint32_t n_e2 = n_e;

        if (cb_idx > cb_segm->C - gamma) {
            n_e2 = n_e + Qm;
            rp = (cb_segm->C - gamma) * n_e + (cb_idx - (cb_segm->C - gamma)) * n_e2;
        }

        // check if we have enough bits for this codeblock available, if not, abort for now
        if (rp + n_e2 > nof_d_bits_maximum) {
            break;
        }

        // descramble all bits required for this codeblock
        if (q->llr_bit_width == 8) {
            srsran_scrambling_sb_offset(srsran_sequence, (int8_t*)&e_bits[rp], rp, n_e2);
        } else if (q->llr_bit_width == 16) {
            srsran_scrambling_s_offset(srsran_sequence, &e_bits[rp], rp, n_e2);
        }

        /* Do not process blocks with CRC Ok */
        if (!softbuffer->cb_crc[cb_idx]) {
            if (q->llr_bit_width == 8) {
                if (srsran_rm_turbo_rx_lut_8bit(&e_bits_b[rp],
                                                (int8_t*)softbuffer->buffer_f[cb_idx],
                                                n_e2,
                                                cb_len_idx,
                                                rv)) {
                    dectnrp_assert_failure("Error in rate matching");
                }
            } else {
                // if (srsran_rm_turbo_rx_lut(&e_bits_s[rp], softbuffer->buffer_f[cb_idx], n_e2,
                // cb_len_idx, rv)) {
                if (srsran_rm_turbo_rx_lut_(
                        &e_bits_s[rp], softbuffer->buffer_f[cb_idx], n_e2, cb_len_idx, rv, false)) {
                    dectnrp_assert_failure("Error in rate matching");
                }
            }

            srsran_tdec_new_cb(&q->decoder, cb_len);

            // Run iterations and use CRC for early stopping
            bool early_stop = false;
            uint32_t cb_noi = 0;
            do {
                if (q->llr_bit_width == 8) {
                    srsran_tdec_iteration_8bit(
                        &q->decoder, (int8_t*)softbuffer->buffer_f[cb_idx], &data[wp / 8]);
                } else {
                    srsran_tdec_iteration(&q->decoder, softbuffer->buffer_f[cb_idx], &data[wp / 8]);
                }
                // ++q->avg_iterations;
                ++cb_noi;

                uint32_t len_crc;
                srsran_crc_t* crc_ptr;

                if (cb_segm->C > 1) {
                    len_crc = cb_len;
                    crc_ptr = &q->crc_cb;
                } else {
                    len_crc = cb_segm->tbs + 24;
                    crc_ptr = &q->crc_tb;
                }

                // CRC is OK and ran the minimum number of iterations
                if (!srsran_crc_checksum_byte(crc_ptr, &data[wp / 8], len_crc) &&
                    (cb_noi >= SRSRAN_PDSCH_MIN_TDEC_ITERS)) {
                    softbuffer->cb_crc[cb_idx] = true;
                    early_stop = true;

                    // CRC is error and exceeded maximum iterations for this CB.
                    // Early stop the whole transport block.
                }

            } while (cb_noi < q->max_iterations && !early_stop);

        } else {
            // Copy decoded data from previous transmissions
            memcpy(&data[wp / 8], softbuffer->data[cb_idx], rlen / 8 * sizeof(uint8_t));
        }

        wp += rlen;

        ++cb_idx;
    }

    // When the last CB was processed, check the overall result.
    // Note that cb_idx++ is executed after cb_idx has reached the value cb_idx = cb_segm->C-1.
    if (cb_idx == cb_segm->C) {
        /* There are these different cases we have to distinguish for our final check:
         *
         * 1) At least one, multiple or all CB CRCs are false:
         *
         * 1.1) cb_segm->C == 1: In this case CB CRC = TB CRC, thus the TB CRC is false as well and
         * we assume incorrect decoding. The incorrectly decoded softbits are still avaiblable in
         * softbuffer->buffer_f[cb_idx] with cb_idx=0.
         *
         * 1.2) cb_segm->C > 1: This means we don't even bother checking the TB CRC. Instead, we
         * save the correctly decoded bits of the CB with true CRCs for the next retransmission. The
         * incorrectly decoded softbits of CBs with false CRCs are still avaiblable in
         * softbuffer->buffer_f[cb_idx].
         *
         * 2) All CB CRC are true:
         *
         * 2.1) cb_segm->C == 1: In this case CB CRC = TB CRC, thus the TB CRC is true as well and
         * we assume correct decoding.
         *
         * 2.2) cb_segm->C > 1: This does not imply that the TB CRC is true, though this is highly
         * probable. We still have to check the TB CRC.
         *
         * 2.2.1) If the TB CRC is true, we assume correct decoding.
         *
         * 2.2.2) If the TB CRC is false, we reset all CB CRCs since at least one, multiple or all
         * CBs show true CRCs despite containing incorrectly decoded bits (false alarm). The
         * incorrectly decoded softbits are still avaiblable in softbuffer->buffer_f[cb_idx].
         */

        // go over each CB and check if any CB CRC is false
        softbuffer->tb_crc = true;
        for (uint32_t i = 0; i < cb_segm->C && softbuffer->tb_crc; ++i) {
            /* If one CB failed return false */
            softbuffer->tb_crc = softbuffer->cb_crc[i];
        }

        // at least one, multiple or all CB CRCs false?
        if (!softbuffer->tb_crc) {
            // if any CB CRC is false, save the correctly decoded CB for next retransmission
            uint32_t write_offset = 0;
            for (uint32_t cb_idx_ = 0; cb_idx_ < cb_segm->C; ++cb_idx_) {
                const uint32_t cb_len = cb_idx_ < cb_segm->C2 ? cb_segm->K2 : cb_segm->K1;
                const uint32_t rlen = cb_segm->C == 1 ? cb_len : (cb_len - 24);

                if (softbuffer->cb_crc[cb_idx_]) {
                    memcpy(softbuffer->data[cb_idx_],
                           &data[write_offset / 8],
                           rlen / 8 * sizeof(uint8_t));
                }

                write_offset += rlen;
            }

            return false;
        }

        // at this point we know that all CB CRC are correct

        // One CB CRC OK, means TB CRC is OK.
        if (cb_segm->C == 1) {
            return true;
        }

        // more than one codeblock, check TB CRC for whole TB
        if (srsran_crc_match_byte(&q->crc_tb, data, cb_segm->tbs)) {
            // all CB CRCs and TB CRC are true, decoding successfull
            return true;
        }

        // TB CRC check failed, as at least one CB had a false alarm, reset all CB CRC flags in the
        // softbuffer
        srsran_softbuffer_rx_reset_cb_crc(softbuffer, cb_segm->C);

        return false;
    }

    return false;
}

}  // namespace dectnrp::section3
