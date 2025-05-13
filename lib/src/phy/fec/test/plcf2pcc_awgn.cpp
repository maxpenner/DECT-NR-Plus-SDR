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

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>

#include "dectnrp/constants.hpp"
#include "dectnrp/phy/fec/fec.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/sections_part3/radio_device_class.hpp"
#include "srsran/srsran.h"

#define SNR_DB_MIN -5.0
#define SNR_DB_MAX 5.0
#define SNR_DB_STEP 0.5

#define N_REPETITIONS 1000

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::srand(time(NULL));

    // define our radio device
    std::string radio_device_class_string("1.1.1.A");
    const auto radio_device_class =
        dectnrp::section3::get_radio_device_class(radio_device_class_string);

    // to allocate memory we need maximum packet sizes
    const auto packet_sizes_maximum =
        dectnrp::section3::get_maximum_packet_sizes(radio_device_class_string);

    // allocate TX buffer
    std::unique_ptr<dectnrp::phy::harq::buffer_tx_t> hb_tx =
        std::make_unique<dectnrp::phy::harq::buffer_tx_t>(
            dectnrp::phy::harq::buffer_tx_t::COMPONENT_T::PLCF);

    // allocate RX buffer
    std::unique_ptr<dectnrp::phy::harq::buffer_rx_plcf_t> hb_rx =
        dectnrp::phy::harq::buffer_rx_plcf_t::new_unique_instance();

    // init fec
    std::unique_ptr<dectnrp::phy::fec_t> fec;
    fec = std::make_unique<dectnrp::phy::fec_t>(packet_sizes_maximum);

    // register network id at fec
    uint32_t network_id = 123456789;
    fec->add_new_network_id(network_id);

    // local test variables
    uint8_t* a_unpacked = srsran_vec_u8_malloc(dectnrp::constants::plcf_type_2_bit);
    uint8_t* a_rx_unpacked = srsran_vec_u8_malloc(dectnrp::constants::plcf_type_2_bit);
    uint8_t* d_unpacked = srsran_vec_u8_malloc(dectnrp::constants::pcc_bits);

    // PLCF type 1 or type 2 bits are turned into 196 bits after channel coding, 196 bits are turned
    // into 196/2=98 complex QPSK symbols
    cf_t* symbols = srsran_vec_cf_malloc(dectnrp::constants::pcc_cells);
    cf_t* symbols_plus_noise = srsran_vec_cf_malloc(dectnrp::constants::pcc_cells);

    // channel
    srsran_channel_awgn_t srsran_channel_awgn;
    srsran_channel_awgn_init(&srsran_channel_awgn, 12345);

    // init modulation table
    srsran_mod_t srsran_mod = SRSRAN_MOD_QPSK;
    srsran_modem_table_t srsran_modem_table;
    srsran_modem_table_lte(&srsran_modem_table, srsran_mod);
    srsran_modem_table_bytes(&srsran_modem_table);

    for (uint32_t PLCF_type = 1; PLCF_type <= 2; ++PLCF_type) {
        for (float SNR_dB = SNR_DB_MIN; SNR_dB <= SNR_DB_MAX; SNR_dB = SNR_dB + SNR_DB_STEP) {
            // see part 4
            const uint32_t N_PLCF_bits = (PLCF_type == 1) ? dectnrp::constants::plcf_type_1_bit
                                                          : dectnrp::constants::plcf_type_2_bit;

            uint64_t uncoded_bit_error = 0;
            uint64_t packet_error = 0;

            double power_signal = 0.0;
            double power_signal_plus_noise = 0.0;

            for (int iter = 0; iter < N_REPETITIONS; ++iter) {
                // reset softbuffer
                hb_tx->reset_a_cnt_and_softbuffer();
                hb_rx->reset_a_cnt_and_softbuffer();

                // write random data
                uint8_t* a_tmp = hb_tx->get_a();
                for (uint32_t i = 0; i < dectnrp::constants::plcf_type_2_bit / 8; ++i) {
                    a_tmp[i] = std::rand() % 256;
                }

                // set TX parameters for this transmission
                dectnrp::section3::fec_cfg_t tx_cfg;
                if (iter % 4 == 0) {
                    tx_cfg.PLCF_type = PLCF_type;
                    tx_cfg.closed_loop = false;
                    tx_cfg.beamforming = false;
                } else if (iter % 4 == 1) {
                    tx_cfg.PLCF_type = PLCF_type;
                    tx_cfg.closed_loop = true;
                    tx_cfg.beamforming = false;
                } else if (iter % 4 == 2) {
                    tx_cfg.PLCF_type = PLCF_type;
                    tx_cfg.closed_loop = false;
                    tx_cfg.beamforming = true;
                } else if (iter % 4 == 3) {
                    tx_cfg.PLCF_type = PLCF_type;
                    tx_cfg.closed_loop = true;
                    tx_cfg.beamforming = true;
                }

                // encode data
                fec->encode_plcf(tx_cfg, *hb_tx.get());

                // transfer 196 data bits from d to d_rx
                srsran_bit_unpack_vector(hb_tx->get_d(), d_unpacked, dectnrp::constants::pcc_bits);
                // last argument is the number of bits
                srsran_mod_modulate(
                    &srsran_modem_table, d_unpacked, symbols, dectnrp::constants::pcc_bits);

                // add noise
                float n0_dBfs = -SNR_dB;
                srsran_channel_awgn_set_n0(&srsran_channel_awgn, n0_dBfs);
                srsran_channel_awgn_run_c(&srsran_channel_awgn,
                                          symbols,
                                          symbols_plus_noise,
                                          dectnrp::constants::pcc_cells);

                // measure SNR
                float* ri_symbols = (float*)symbols;
                float* ri_symbols_plus_noise = (float*)symbols_plus_noise;
                for (uint32_t s_idx = 0; s_idx < dectnrp::constants::pcc_cells; ++s_idx) {
                    power_signal +=
                        pow(ri_symbols[2 * s_idx], 2.0) + pow(ri_symbols[2 * s_idx + 1], 2.0);
                    power_signal_plus_noise += pow(ri_symbols_plus_noise[2 * s_idx], 2.0) +
                                               pow(ri_symbols_plus_noise[2 * s_idx + 1], 2.0);
                }

                // demodulate
                uint8_t* d_tmp = hb_rx->get_d();
                // 196 bits, 2 bits per symbol für QPSK, 192/2=98 symbols
                srsran_demod_soft_demodulate_s(
                    srsran_mod, symbols_plus_noise, (short*)d_tmp, dectnrp::constants::pcc_cells);

                // decode data
                dectnrp::section3::fec_cfg_t rx_cfg;
                fec->decode_plcf_test(rx_cfg, *hb_rx.get(), PLCF_type);

                // count uncoded bit errors
                PHY_D_RX_DATA_TYPE* tmp_ = (PHY_D_RX_DATA_TYPE*)hb_rx->get_d();
                for (uint32_t i = 0; i < dectnrp::constants::pcc_bits; ++i) {
                    int signA = (d_unpacked[i] > 0) ? 1 : -1;
                    int signB = (tmp_[i] > 0) ? 1 : -1;
                    if (signA != signB) {
                        ++uncoded_bit_error;
                    }
                }

                bool any_error = false;

                // compare data
                srsran_bit_unpack_vector(hb_tx->get_a(), a_unpacked, N_PLCF_bits);
                srsran_bit_unpack_vector(hb_rx->get_a(), a_rx_unpacked, N_PLCF_bits);
                for (uint32_t a_idx = 0; a_idx < N_PLCF_bits; ++a_idx) {
                    if (a_unpacked[a_idx] != a_rx_unpacked[a_idx]) {
                        any_error = true;
                    }
                }

                if (any_error) {
                    ++packet_error;
                }

                // compare cfg data
                if (tx_cfg.PLCF_type != rx_cfg.PLCF_type ||
                    tx_cfg.closed_loop != rx_cfg.closed_loop ||
                    tx_cfg.beamforming != rx_cfg.beamforming) {
                }
            }  // iter

            double SNR_dB_measured =
                10.0 * log10(power_signal / (power_signal_plus_noise - power_signal));
            double BER_uncoded =
                (double)uncoded_bit_error / (double)(dectnrp::constants::pcc_bits * N_REPETITIONS);
            double PER = (double)packet_error / (double)(N_REPETITIONS);

            std::cout << std::fixed << std::setprecision(5) << std::setfill(' ');
            std::cout << " N_PLCF_bits=" << N_PLCF_bits;
            std::cout << " SNR_dB=" << SNR_dB;
            std::cout << " SNR_dB_measured=" << SNR_dB_measured;
            std::cout << " BER_uncoded=" << BER_uncoded;
            std::cout << " packet_error=" << std::setw(7) << packet_error;
            std::cout << " PER=" << PER;
            std::cout << std::endl;

        }  // N_PLCF_bits
    }  // snr

    // clean up
    srsran_modem_table_free(&srsran_modem_table);

    free(symbols);
    free(symbols_plus_noise);

    srsran_channel_awgn_free(&srsran_channel_awgn);

    free(a_unpacked);
    free(a_rx_unpacked);
    free(d_unpacked);

    return EXIT_SUCCESS;
}
