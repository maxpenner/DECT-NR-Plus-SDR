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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/common/randomgen.hpp"
#include "dectnrp/phy/fec/fec.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"
#include "dectnrp/sections_part3/radio_device_class.hpp"
#include "header_only/nlohmann/json.hpp"
#include "srsran/srsran.h"

#define MCS_MIN 0
#define MCS_MAX 9

#define SNR_DB_MIN -15.0
#define SNR_DB_MAX 25.0
#define SNR_DB_STEP 2.0

#define N_HARQ_RETX_MIN 0
#define N_HARQ_RETX_MAX 3

#define N_PACKETS 5

#define G_STEP_MIN_BITS 33U

#define PACKET_CORRECTNESS_BASED_ON_CRC_OR_CODED_BIT_COMPARISON

static uint32_t json_file_cnt = 0;

int run_test(std::string radio_device_class_string, const uint32_t packet_length_in_slots) {
    dectnrp::common::randomgen_t randomgen;
    randomgen.shuffle();

    // define radio class
    const auto radio_device_class =
        dectnrp::section3::get_radio_device_class(radio_device_class_string);

    // to allocate memory we need to know maximum packet sizes in advance
    const auto packet_sizes_maximum =
        dectnrp::section3::get_maximum_packet_sizes(radio_device_class_string);

    // allocate largest conceivable TX buffer
    std::unique_ptr<dectnrp::phy::harq::buffer_tx_t> hb_tx =
        std::make_unique<dectnrp::phy::harq::buffer_tx_t>(
            dectnrp::phy::harq::buffer_tx_t::COMPONENT_T::TRANSPORT_BLOCK,
            packet_sizes_maximum.N_TB_byte,
            packet_sizes_maximum.G,
            packet_sizes_maximum.C,
            packet_sizes_maximum.psdef.Z);

    // allocate largest conceivable RX buffer
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

    cf_t* const symbols = srsran_vec_cf_malloc(packet_sizes_maximum.N_PDC_subc);
    cf_t* const symbols_plus_noise = srsran_vec_cf_malloc(packet_sizes_maximum.N_PDC_subc);

    // channel
    srsran_channel_awgn_t srsran_channel_awgn;
    srsran_channel_awgn_init(&srsran_channel_awgn, std::rand() % UINT32_MAX);

    // MSC can be limited by device class
    for (uint32_t mcs = MCS_MIN;
         mcs <= std::min(radio_device_class.mcs_index_min, (uint32_t)MCS_MAX);
         ++mcs) {
        // define a transmission
        const dectnrp::section3::packet_sizes_def_t psdef = {
            .u = 1,
            .b = 1,
            .PacketLengthType = 1,
            .PacketLength = std::min(radio_device_class.PacketLength_min, packet_length_in_slots),
            .tm_mode_index = 0,
            .mcs_index = mcs,
            .Z = radio_device_class.Z_min};

        // calculate sizes of this transmission
        const auto packet_sizes = dectnrp::section3::get_packet_sizes(psdef);

        if (!packet_sizes.has_value()) {
            dectnrp_print_wrn("Impossible packet sizes configuration.");
            return EXIT_FAILURE;
        }

        uint32_t N_TB_bits = packet_sizes->N_TB_bits;
        uint32_t N_TB_byte = packet_sizes->N_TB_byte;
        uint32_t N_PDC_subc = packet_sizes->N_PDC_subc;
        uint32_t N_bps = packet_sizes->mcs.N_bps;
        uint32_t G = packet_sizes->G;

        // set TX and RX cfg parameters for this transmission
        dectnrp::section3::fec_cfg_t tx_cfg = {
            .PLCF_type = randomgen.randi(1, 2),
            .closed_loop = (randomgen.randi(0, 1) > 0) ? true : false,
            .beamforming = (randomgen.randi(0, 1) > 0) ? true : false,
            .N_TB_bits = N_TB_bits,
            .N_bps = N_bps,
            .rv = 0,
            .G = G,
            .network_id = network_id,
            .Z = psdef.Z};

        // assume same data is known at receiver, usually extracted from PLCF
        dectnrp::section3::fec_cfg_t rx_cfg = {.PLCF_type = tx_cfg.PLCF_type,
                                               .closed_loop = tx_cfg.closed_loop,
                                               .beamforming = tx_cfg.beamforming,
                                               .N_TB_bits = tx_cfg.N_TB_bits,
                                               .N_bps = tx_cfg.N_bps,
                                               .rv = tx_cfg.rv,
                                               .G = tx_cfg.G,
                                               .network_id = tx_cfg.network_id,
                                               .Z = tx_cfg.Z};

        // convert mcs to srsRAN terminology
        srsran_mod_t srsran_mod;
        if (mcs == 0) {
            srsran_mod = SRSRAN_MOD_BPSK;
        } else if (mcs == 1 || mcs == 2) {
            srsran_mod = SRSRAN_MOD_QPSK;
        } else if (mcs == 3 || mcs == 4) {
            srsran_mod = SRSRAN_MOD_16QAM;
        } else if (mcs == 5 || mcs == 6 || mcs == 7) {
            srsran_mod = SRSRAN_MOD_64QAM;
        } else if (mcs == 8 || mcs == 9) {
            srsran_mod = SRSRAN_MOD_256QAM;
        } else {
            dectnrp_print_wrn("Unknown MCS.");
            return EXIT_FAILURE;
        }

        // init modulation table
        srsran_modem_table_t srsran_modem_table;
        srsran_modem_table_lte(&srsran_modem_table, srsran_mod);
        srsran_modem_table_bytes(&srsran_modem_table);

        for (uint32_t N_HARQ_RETX = N_HARQ_RETX_MIN; N_HARQ_RETX <= N_HARQ_RETX_MAX;
             ++N_HARQ_RETX) {
            // these vector will later be saved to a json file
            std::vector<double> SNR_dB_vec;
            std::vector<double> SNR_dB_measured_vec;
            std::vector<double> BER_uncoded_vec;
            std::vector<double> PER_vec;

            for (double SNR_dB = SNR_DB_MIN; SNR_dB <= SNR_DB_MAX; SNR_dB += SNR_DB_STEP) {
                uint64_t uncoded_bit_error = 0;
                uint64_t packet_transmissions = 0;
                uint64_t packet_error = 0;

                double power_signal = 0.0;
                double power_signal_plus_noise = 0.0;

                for (uint32_t iter = 0; iter < N_PACKETS; ++iter) {
                    // must be done for each new transmission
                    hb_tx->reset_a_cnt_and_softbuffer();
                    hb_rx->reset_a_cnt_and_softbuffer();

                    // write random data
                    uint8_t* a_tmp = hb_tx->get_a();
                    for (uint32_t i = 0; i < N_TB_byte; ++i) {
                        a_tmp[i] = std::rand() % 256;
                    }
                    srsran_bit_unpack_vector(hb_tx->get_a(), a_unpacked, tx_cfg.N_TB_bits);

                    bool packet_correct = false;

                    // send retransmission until packet correct or N_HARQ_RETX reached
                    for (uint32_t transmission = 0; transmission < (1 + N_HARQ_RETX);
                         ++transmission) {
                        ++packet_transmissions;

                        switch (transmission) {
                            case 0:
                                tx_cfg.rv = 0;
                                break;
                            case 1:
                                tx_cfg.rv = 2;
                                break;
                            case 2:
                                tx_cfg.rv = 3;
                                break;
                            case 3:
                                tx_cfg.rv = 1;
                                break;
                        }
                        rx_cfg.rv = tx_cfg.rv;

                        // prepare channel encoding
                        fec->segmentate_and_pick_scrambling_sequence(tx_cfg);

                        // encode data
                        uint32_t G_tx_cnt = 0;
                        do {
                            G_tx_cnt += std::min(G_STEP_MIN_BITS, tx_cfg.G - G_tx_cnt);
                            fec->encode_tb(tx_cfg, *hb_tx.get(), G_tx_cnt);
                        } while (G_tx_cnt < tx_cfg.G);

                        // modulate
                        srsran_bit_unpack_vector(hb_tx->get_d(), d_unpacked, tx_cfg.G);
                        srsran_mod_modulate(&srsran_modem_table, d_unpacked, symbols, tx_cfg.G);

                        // add noise
                        float n0_dBfs = (0 - SNR_dB) * 1.0;
                        srsran_channel_awgn_set_n0(&srsran_channel_awgn, n0_dBfs);
                        srsran_channel_awgn_run_c(
                            &srsran_channel_awgn, symbols, symbols_plus_noise, N_PDC_subc);

                        // measure SNR
                        float* ri_symbols = (float*)symbols;
                        float* ri_symbols_plus_noise = (float*)symbols_plus_noise;
                        for (uint32_t s_idx = 0; s_idx < N_PDC_subc; ++s_idx) {
                            power_signal += pow(ri_symbols[2 * s_idx], 2.0) +
                                            pow(ri_symbols[2 * s_idx + 1], 2.0);
                            power_signal_plus_noise +=
                                pow(ri_symbols_plus_noise[2 * s_idx], 2.0) +
                                pow(ri_symbols_plus_noise[2 * s_idx + 1], 2.0);
                        }

                        // demodulate
                        uint8_t* const d_tmp = hb_rx->get_d();
                        srsran_demod_soft_demodulate_s(
                            srsran_mod, symbols_plus_noise, (short*)d_tmp, N_PDC_subc);

                        // count uncoded bit errors
                        PHY_D_RX_DATA_TYPE* const tmp = (PHY_D_RX_DATA_TYPE*)hb_rx->get_d();
                        for (uint32_t i = 0; i < G; ++i) {
                            int signA = (d_unpacked[i] > 0) ? 1 : -1;
                            int signB = (tmp[i] > 0) ? 1 : -1;
                            if (signA != signB) {
                                ++uncoded_bit_error;
                            }
                        }

                        // prepare channel decoding
                        fec->segmentate_and_pick_scrambling_sequence(rx_cfg);

                        // decode data
                        uint32_t G_rx_cnt = 0;
                        do {
                            uint32_t G_step = std::min(G_STEP_MIN_BITS, tx_cfg.G - G_rx_cnt);
                            G_rx_cnt += G_step;
                            fec->decode_tb(tx_cfg, *hb_rx.get(), G_rx_cnt);
                        } while (G_rx_cnt < tx_cfg.G);

#ifdef PACKET_CORRECTNESS_BASED_ON_CRC_OR_CODED_BIT_COMPARISON
                        if (fec->get_decode_tb_status_latest()) {
                            packet_correct = true;
                            break;
                        }
#else
                        // compare data
                        uint64_t coded_bit_error = 0;
                        srsran_bit_unpack_vector(hb_rx->get_a(), a_rx_unpacked, tx_cfg.N_TB_bits);
                        for (uint32_t a_idx = 0; a_idx < N_TB_bits; ++a_idx) {
                            if (a_unpacked[a_idx] != a_rx_unpacked[a_idx]) {
                                ++coded_bit_error;
                            }
                        }

                        // abort harq transmission prematurely
                        if (coded_bit_error == 0) {
                            packet_correct = true;
                            break;
                        }
#endif
                    }  // harq

                    if (!packet_correct) {
                        ++packet_error;
                    }
                }  // iter

                double SNR_dB_measured =
                    10.0 * log10(power_signal / (power_signal_plus_noise - power_signal));
                double BER_uncoded = (double)uncoded_bit_error / (double)(G * packet_transmissions);
                double PER = (double)packet_error / (double)(N_PACKETS);

                SNR_dB_vec.push_back(SNR_dB);
                SNR_dB_measured_vec.push_back(SNR_dB_measured);
                BER_uncoded_vec.push_back(BER_uncoded);
                PER_vec.push_back(PER);

                // show progress in tpoint
                std::cout << std::fixed << std::setprecision(5) << std::setfill(' ');
                std::cout << "Z=" << tx_cfg.Z;
                std::cout << " MCS=" << mcs;
                std::cout << " N_HARQ_RETX=" << N_HARQ_RETX;
                std::cout << " SNR_dB=" << SNR_dB;
                std::cout << " SNR_dB_measured=" << SNR_dB_measured;
                std::cout << " BER_uncoded=" << BER_uncoded;
                std::cout << " packet_error=" << packet_error;
                std::cout << " PER=" << PER;
                std::cout << " N_TB_bits=" << N_TB_bits;
                std::cout << " N_PDC_subc=" << N_PDC_subc;
                std::cout << " G=" << G;
                std::cout << std::endl;
            }

            // save all data to json file
            std::ostringstream filename;
            filename << "fec_packet_" << std::setw(10) << std::setfill('0') << json_file_cnt++;

            nlohmann::ordered_json j_packet_data;

            j_packet_data["identifier"] = json_file_cnt;
            j_packet_data["radio_device_class_string"] = radio_device_class_string;
            j_packet_data["PacketLength"] = psdef.PacketLength;
            j_packet_data["Z"] = tx_cfg.Z;
            j_packet_data["MCS"] = mcs;
            j_packet_data["N_HARQ_RETX"] = N_HARQ_RETX;
            j_packet_data["N_TB_bits"] = N_TB_bits;
            j_packet_data["N_PDC_subc"] = N_PDC_subc;
            j_packet_data["G"] = G;
            j_packet_data["N_PACKETS"] = N_PACKETS;

            j_packet_data["data"]["SNR_dB_vec"] = SNR_dB_vec;
            j_packet_data["data"]["SNR_dB_measured_vec"] = SNR_dB_measured_vec;
            j_packet_data["data"]["BER_uncoded_vec"] = BER_uncoded_vec;
            j_packet_data["data"]["PER_vec"] = PER_vec;

            std::ofstream out_file(filename.str());
            out_file << std::setw(4) << j_packet_data << std::endl;
            out_file.close();

        }  // harq
        srsran_modem_table_free(&srsran_modem_table);
    }  // mcs

    srsran_channel_awgn_free(&srsran_channel_awgn);

    free(symbols);
    free(symbols_plus_noise);

    free(a_unpacked);
    free(a_rx_unpacked);
    free(d_unpacked);

    return EXIT_SUCCESS;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::srand(time(NULL));

    uint32_t packet_length_in_slots = 1;

    // small TBS, Z = 2048
    if (run_test("1.1.1.A", packet_length_in_slots)) {
        return EXIT_FAILURE;
    }

    // small TBS, Z = 6144
    if (run_test("1.1.1.B", packet_length_in_slots)) {
        return EXIT_FAILURE;
    }

    packet_length_in_slots = 16;

    // large TBS, Z = 2048
    if (run_test("2.12.4.A", packet_length_in_slots)) {
        return EXIT_FAILURE;
    }

    // large TBS, Z = 6144
    if (run_test("2.12.4.B", packet_length_in_slots)) {
        return EXIT_FAILURE;
    }
}
