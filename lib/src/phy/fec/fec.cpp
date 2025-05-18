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

#include "dectnrp/phy/fec/fec.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part3/fix/cbsegm.hpp"

namespace dectnrp::phy {

fec_t::fec_t(const section3::packet_sizes_t& packet_sizes_maximum) {
    if (section3::pcc_enc_init(&pcc_enc) == SRSRAN_ERROR) {
        dectnrp_assert_failure("Unable to init FEC for PCC.");
    }

    if (section3::pdc_enc_init(&pdc_enc, packet_sizes_maximum) == SRSRAN_ERROR) {
        dectnrp_assert_failure("Unable to init FEC for PDC.");
    }

    scrambling_pdc.set_maximum_sequence_length_G(packet_sizes_maximum.G);
}

fec_t::~fec_t() {
    section3::pcc_enc_free(&pcc_enc);
    section3::pdc_enc_free(&pdc_enc);
}

void fec_t::add_new_network_id(const uint32_t network_id) { scrambling_pdc.add(network_id); }

void fec_t::encode_plcf(const section3::fec_cfg_t& tx_cfg, harq::buffer_tx_t& hb) {
    section3::pcc_enc_encode(&pcc_enc,
                             hb.get_a(),
                             hb.get_d(),
                             hb.get_softbuffer_d(),
                             tx_cfg.PLCF_type,
                             tx_cfg.closed_loop,
                             tx_cfg.beamforming);
}

bool fec_t::decode_plcf_test(section3::fec_cfg_t& rx_cfg,
                             harq::buffer_rx_plcf_t& hb,
                             const uint32_t PLCF_type_test) {
    // regardless of the result, overwrite with tested PLCF type
    rx_cfg.PLCF_type = PLCF_type_test;

    dectnrp_assert(PLCF_type_test == 1 || PLCF_type_test == 2, "Unknown PLCF type");

    // hb for PLCF internally has two softbuffers for both types, pick softbuffer for tested type
    srsran_softbuffer_rx_t* softbuffer_ptr = nullptr;
    if (PLCF_type_test == 1) {
        softbuffer_ptr = hb.get_softbuffer_d_type_1();
    } else if (PLCF_type_test == 2) {
        softbuffer_ptr = hb.get_softbuffer_d_type_2();
    }

    return section3::pcc_enc_decode(&pcc_enc,
                                    hb.get_a(),
                                    hb.get_d(),
                                    softbuffer_ptr,
                                    PLCF_type_test,
                                    rx_cfg.closed_loop,
                                    rx_cfg.beamforming);
}

void fec_t::segmentate_and_pick_scrambling_sequence(const section3::fec_cfg_t& tx_cfg) {
    // codeblock segmentation
    if (section3::fix::srsran_cbsegm_FIX(&srsran_cbsegm, tx_cfg.N_TB_bits, tx_cfg.Z) ==
        SRSRAN_ERROR) {
        dectnrp_assert_failure("Error computing codeblock segmentation");
    }

    // get pointer to scrambling sequence
    srsran_sequence = scrambling_pdc.get(tx_cfg.network_id, tx_cfg.PLCF_type);

    // reset TB CRC
    srsran_crc_set_init(&pdc_enc.crc_tb, 0);

    // reset status
    decode_tb_status_latest = false;

    // reset counters
    cb_idx = 0;
    rp = 0;
    wp = 0;
}

void fec_t::encode_tb(const section3::fec_cfg_t& tx_cfg, harq::buffer_tx_t& hb) {
    encode_tb(tx_cfg, hb, UINT32_MAX);
}

void fec_t::encode_tb(const section3::fec_cfg_t& tx_cfg,
                      harq::buffer_tx_t& hb,
                      const uint32_t nof_bits_minimum) {
    uint32_t nof_d_bits_minimum = std::min(nof_bits_minimum, tx_cfg.G);

    section3::pdc_encode_codeblocks(&pdc_enc,
                                    hb.get_softbuffer_d(),
                                    &srsran_cbsegm,
                                    tx_cfg.N_bps,
                                    tx_cfg.rv,
                                    tx_cfg.G,
                                    hb.get_a(),
                                    hb.get_d(),
                                    cb_idx,
                                    rp,
                                    wp,
                                    nof_d_bits_minimum,
                                    srsran_sequence);
}

void fec_t::decode_tb(const section3::fec_cfg_t& rx_cfg, harq::buffer_rx_t& hb) {
    decode_tb(rx_cfg, hb, UINT32_MAX);
}

void fec_t::decode_tb(const section3::fec_cfg_t& rx_cfg,
                      harq::buffer_rx_t& hb,
                      const uint32_t nof_bits_maximum) {
    dectnrp_assert(nof_bits_maximum <= rx_cfg.G, "Nof bits fed larger G");

    decode_tb_status_latest = pdc_decode_codeblocks(&pdc_enc,
                                                    hb.get_softbuffer_d(),
                                                    &srsran_cbsegm,
                                                    rx_cfg.N_bps,
                                                    rx_cfg.rv,
                                                    rx_cfg.G,
                                                    (int16_t*)hb.get_d(),
                                                    hb.get_a(),
                                                    cb_idx,
                                                    wp,
                                                    nof_bits_maximum,
                                                    srsran_sequence);
}

}  // namespace dectnrp::phy
