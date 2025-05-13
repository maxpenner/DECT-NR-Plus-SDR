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

#include <cmath>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <memory>

#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/fec/fec.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/sections_part3/radio_device_class.hpp"
#include "srsran/srsran.h"

#define N_REPETITIONS 10000

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
    bool any_error = false;
    uint8_t* a_unpacked = srsran_vec_u8_malloc(dectnrp::constants::plcf_type_2_bit);
    uint8_t* a_rx_unpacked = srsran_vec_u8_malloc(dectnrp::constants::plcf_type_2_bit);
    uint8_t* d_unpacked = srsran_vec_u8_malloc(dectnrp::constants::pcc_bits);

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
        if (iter % 8 == 0) {
            tx_cfg.PLCF_type = 1;
            tx_cfg.closed_loop = false;
            tx_cfg.beamforming = false;
        } else if (iter % 8 == 1) {
            tx_cfg.PLCF_type = 1;
            tx_cfg.closed_loop = true;
            tx_cfg.beamforming = false;
        } else if (iter % 8 == 2) {
            tx_cfg.PLCF_type = 1;
            tx_cfg.closed_loop = false;
            tx_cfg.beamforming = true;
        } else if (iter % 8 == 3) {
            tx_cfg.PLCF_type = 1;
            tx_cfg.closed_loop = true;
            tx_cfg.beamforming = true;
        } else if (iter % 8 == 4) {
            tx_cfg.PLCF_type = 2;
            tx_cfg.closed_loop = true;
            tx_cfg.beamforming = false;
        } else if (iter % 8 == 5) {
            tx_cfg.PLCF_type = 2;
            tx_cfg.closed_loop = true;
            tx_cfg.beamforming = false;
        } else if (iter % 8 == 6) {
            tx_cfg.PLCF_type = 2;
            tx_cfg.closed_loop = false;
            tx_cfg.beamforming = true;
        } else if (iter % 8 == 7) {
            tx_cfg.PLCF_type = 2;
            tx_cfg.closed_loop = true;
            tx_cfg.beamforming = true;
        }

        // encode data
        fec->encode_plcf(tx_cfg, *hb_tx.get());

        // transfer 196 bits from d to d_rx
        srsran_bit_unpack_vector(hb_tx->get_d(), d_unpacked, dectnrp::constants::pcc_bits);
        PHY_D_RX_DATA_TYPE* tmp = (PHY_D_RX_DATA_TYPE*)hb_rx->get_d();
        for (uint32_t i = 0; i < dectnrp::constants::pcc_bits; ++i) {
            tmp[i] = (d_unpacked[i] > 0) ? 127 : -127;
        }

        // decode data
        dectnrp::section3::fec_cfg_t rx_cfg;
        fec->decode_plcf_test(rx_cfg, *hb_rx.get(), tx_cfg.PLCF_type);

        // compare bits
        uint32_t N_PLCF_bits = (tx_cfg.PLCF_type == 1) ? dectnrp::constants::plcf_type_1_bit
                                                       : dectnrp::constants::plcf_type_2_bit;
        srsran_bit_unpack_vector(hb_tx->get_a(), a_unpacked, N_PLCF_bits);
        srsran_bit_unpack_vector(hb_rx->get_a(), a_rx_unpacked, N_PLCF_bits);
        for (uint32_t a_idx = 0; a_idx < N_PLCF_bits; ++a_idx) {
            if (a_unpacked[a_idx] != a_rx_unpacked[a_idx]) {
                any_error = true;
            }
        }

        // compare cfg data
        if (tx_cfg.PLCF_type != rx_cfg.PLCF_type || tx_cfg.closed_loop != rx_cfg.closed_loop ||
            tx_cfg.beamforming != rx_cfg.beamforming) {
            any_error = true;
        }
    }

    if (any_error) {
        dectnrp_print_wrn(
            "PHY PLCF Coding Test failed. This can happen when blindly decoding type 1 or 2.");
    } else {
        dectnrp_print_inf("PHY PLCF Coding Test passed.");
    }

    free(a_unpacked);
    free(a_rx_unpacked);
    free(d_unpacked);

    if (any_error) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
