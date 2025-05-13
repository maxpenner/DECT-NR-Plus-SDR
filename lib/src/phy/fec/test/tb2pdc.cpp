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

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

#include "dectnrp/phy/fec/fec.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"
#include "dectnrp/sections_part3/radio_device_class.hpp"
#include "srsran/srsran.h"

int run_test(std::string radio_device_class_string) {
    // define radio class
    const auto radio_device_class =
        dectnrp::section3::get_radio_device_class(radio_device_class_string);

    // to allocate memory we need to know maximum packet sizes in advance
    const auto packet_sizes_maximum =
        dectnrp::section3::get_maximum_packet_sizes(radio_device_class_string);

    // allocate TX buffer
    std::unique_ptr<dectnrp::phy::harq::buffer_tx_t> hb_tx =
        std::make_unique<dectnrp::phy::harq::buffer_tx_t>(
            dectnrp::phy::harq::buffer_tx_t::COMPONENT_T::TRANSPORT_BLOCK,
            packet_sizes_maximum.N_TB_byte,
            packet_sizes_maximum.G,
            packet_sizes_maximum.C,
            packet_sizes_maximum.psdef.Z);

    // allocate RX buffer
    std::unique_ptr<dectnrp::phy::harq::buffer_rx_t> hb_rx =
        std::make_unique<dectnrp::phy::harq::buffer_rx_t>(packet_sizes_maximum.N_TB_byte,
                                                          packet_sizes_maximum.G,
                                                          packet_sizes_maximum.C,
                                                          packet_sizes_maximum.psdef.Z);

    // init fec
    std::unique_ptr<dectnrp::phy::fec_t> fec =
        std::make_unique<dectnrp::phy::fec_t>(packet_sizes_maximum);

    // register network id at fec
    const uint32_t network_id = 123456789;
    fec->add_new_network_id(network_id);
    fec->add_new_network_id(network_id - 1);
    fec->add_new_network_id(network_id - 2);
    fec->add_new_network_id(network_id - 3);

    // required for testing
    uint8_t* const a_unpacked = srsran_vec_u8_malloc(packet_sizes_maximum.N_TB_bits);
    uint8_t* const a_rx_unpacked = srsran_vec_u8_malloc(packet_sizes_maximum.N_TB_bits);
    uint8_t* const d_unpacked = srsran_vec_u8_malloc(packet_sizes_maximum.G);

    bool any_error = false;

    for (uint32_t u = 1; u <= radio_device_class.u_min; u = u * 2) {
        const uint32_t b_idx_max = dectnrp::section3::phyres::b2b_idx[radio_device_class.b_min];

        for (uint32_t b_idx = 0; b_idx <= b_idx_max; ++b_idx) {
            const uint32_t b = dectnrp::section3::phyres::b_idx2b[b_idx];

            const uint32_t t_max =
                dectnrp::section3::tmmode::get_max_tm_mode_index_depending_on_N_TX(
                    radio_device_class.N_TX_min);

            for (uint32_t t = 0; t <= t_max; ++t) {
                for (uint32_t p_type = 0; p_type <= 1; ++p_type) {
                    const uint32_t p_start = 1;
                    const uint32_t p_step = 2;
                    const uint32_t p_end = std::min(radio_device_class.PacketLength_min, 16U);

                    for (uint32_t p = p_start; p <= p_end; p += p_step) {
                        const uint32_t mcs_step = 2;

                        for (uint32_t mcs = 0; mcs <= radio_device_class.mcs_index_min;
                             mcs += mcs_step) {
                            for (uint32_t PLCF_type = 1; PLCF_type <= 2; ++PLCF_type) {
                                const uint32_t iter_max = 5;

                                for (uint32_t iter = 0; iter < iter_max; ++iter) {
                                    // define a transmission
                                    const dectnrp::section3::packet_sizes_def_t psdef = {
                                        .u = u,
                                        .b = b,
                                        .PacketLengthType = p_type,
                                        .PacketLength = p,
                                        .tm_mode_index = t,
                                        .mcs_index = mcs,
                                        .Z = radio_device_class.Z_min};

                                    // calculate sizes of this transmission
                                    const auto packet_sizes =
                                        dectnrp::section3::get_packet_sizes(psdef);

                                    // is packet configuration valid?
                                    if (!packet_sizes.has_value()) {
                                        continue;
                                    }

                                    // srsran has a size limitation
                                    if (packet_sizes->C > SRSRAN_MAX_CODEBLOCKS) {
                                        continue;
                                    }

                                    const uint32_t N_TB_bits = packet_sizes->N_TB_bits;
                                    const uint32_t N_TB_byte = packet_sizes->N_TB_byte;
                                    const uint32_t N_bps = packet_sizes->mcs.N_bps;
                                    const uint32_t G = packet_sizes->G;

                                    // write random data
                                    uint8_t* a_tmp = hb_tx->get_a();
                                    for (uint32_t i = 0; i < N_TB_byte; ++i) {
                                        a_tmp[i] = std::rand() % 256;
                                    }

                                    // must be done for each new transmission
                                    hb_tx->reset_a_cnt_and_softbuffer();
                                    hb_rx->reset_a_cnt_and_softbuffer();

                                    // set TX and RX cfg parameters for this transmission
                                    const dectnrp::section3::fec_cfg_t tx_cfg = {
                                        .PLCF_type = PLCF_type,
                                        .closed_loop = true,
                                        .beamforming = true,
                                        .N_TB_bits = N_TB_bits,
                                        .N_bps = N_bps,
                                        .rv = 0,
                                        .G = G,
                                        .network_id = network_id,
                                        .Z = psdef.Z};

                                    // assume same data is known at receiver, usually extracted from
                                    // PLCF
                                    const dectnrp::section3::fec_cfg_t rx_cfg = {
                                        .PLCF_type = tx_cfg.PLCF_type,
                                        .closed_loop = tx_cfg.closed_loop,
                                        .beamforming = tx_cfg.beamforming,
                                        .N_TB_bits = tx_cfg.N_TB_bits,
                                        .N_bps = tx_cfg.N_bps,
                                        .rv = tx_cfg.rv,
                                        .G = tx_cfg.G,
                                        .network_id = tx_cfg.network_id,
                                        .Z = tx_cfg.Z};

                                    // prepare channel encoding
                                    fec->segmentate_and_pick_scrambling_sequence(tx_cfg);

                                    // encode data
                                    fec->encode_tb(tx_cfg, *hb_tx.get());

                                    // transfer data from d to d_rx
                                    srsran_bit_unpack_vector(hb_tx->get_d(), d_unpacked, tx_cfg.G);
                                    PHY_D_RX_DATA_TYPE* tmp = (PHY_D_RX_DATA_TYPE*)hb_rx->get_d();
                                    for (uint32_t i = 0; i < tx_cfg.G; ++i) {
                                        tmp[i] = (d_unpacked[i] > 0) ? 10 : -10;
                                    }

                                    // prepare channel decoding
                                    fec->segmentate_and_pick_scrambling_sequence(rx_cfg);

                                    // decode data
                                    fec->decode_tb(rx_cfg, *hb_rx.get(), rx_cfg.G);

                                    // compare data
                                    srsran_bit_unpack_vector(
                                        hb_tx->get_a(), a_unpacked, tx_cfg.N_TB_bits);
                                    srsran_bit_unpack_vector(
                                        hb_rx->get_a(), a_rx_unpacked, tx_cfg.N_TB_bits);
                                    for (uint32_t a_idx = 0; a_idx < N_TB_bits; ++a_idx) {
                                        if (a_unpacked[a_idx] != a_rx_unpacked[a_idx]) {
                                            any_error = true;
                                            break;
                                        }
                                    }
                                }  // iter
                            }  // N_PLCF_bits
                        }  // mcs
                    }  // p
                }  // p_type
            }  // t
        }  // b
    }  // u

    // clean up

    free(a_unpacked);
    free(a_rx_unpacked);
    free(d_unpacked);

    if (any_error) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::srand(time(NULL));

    const std::vector<std::string> rdc_vec = {
        "1.1.1.A", "8.1.1.A", "1.8.1.A", "2.8.2.A", "2.12.4.A",  //"8.12.8.A", "8.16.8.A"
    };

    for (auto rdc : rdc_vec) {
        if (run_test(rdc)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
