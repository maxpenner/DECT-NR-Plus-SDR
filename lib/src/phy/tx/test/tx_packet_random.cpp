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

#include <cstdlib>
#include <memory>

#include "dectnrp/common/adt/freq_shift.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/common/randomgen.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/harq/process_pool.hpp"
#include "dectnrp/phy/phy_config.hpp"
#include "dectnrp/phy/resample/resampler.hpp"
#include "dectnrp/phy/tx/tx.hpp"
#include "dectnrp/phy/tx/tx_descriptor.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"
#include "dectnrp/simulation/vspace.hpp"

#define APPLY_RANDOM_PHASE_AND_FREQUENCY_SHIFT_AT_TX

int generate_random_TX_packet_within_rdc_limits(std::string radio_device_class_string) {
    dectnrp::common::randomgen_t randomgen;
    randomgen.shuffle();

    // to allocate memory we need to know maximum packet sizes in advance
    const auto packet_sizes_maximum =
        dectnrp::sp3::get_maximum_packet_sizes(radio_device_class_string);

    // create HARQ buffer pool for TX and RX
    auto hpp = std::make_unique<dectnrp::phy::harq::process_pool_t>(packet_sizes_maximum, 1, 1);

    // oversampling is not a DECT NR+ variable
    const uint32_t os_min = randomgen.randi(1, 2);

    // with resampling given, what will the maximum oversampled DECT NR+ sample rate be?
    const uint32_t dect_samp_rate_os = packet_sizes_maximum.numerology.B_u_b_DFT * os_min;

    // resampling is not a DECT NR+ variable
    const uint32_t use_resampling = randomgen.randi(0, 1);
    uint32_t L, M;
    if (use_resampling == 0) {
        L = 1;
        M = 1;
    } else {
        // resampling for an USRP
        if (packet_sizes_maximum.psdef.b != 12) {
            L = 10;
            M = 9;
        } else {
            L = 40;
            M = 27;
        }
    }

    // initial configuration of hardware (only set fields required, usually read from file)
    const dectnrp::radio::hw_config_t hw_config = {.nof_buffer_tx = 1, .rx_prestream_ms = 0};
    dectnrp::radio::hw_config_t::sim_samp_rate_lte = (L == 1) ? false : true;

    // create dummy virtual space (required to initialize hw_simulator)
    dectnrp::simulation::vspace_t vspace(123, 123, "awgn", "awgn", "relative");

    // create hw_simulator
    std::unique_ptr<dectnrp::radio::hw_simulator_t> hw =
        std::make_unique<dectnrp::radio::hw_simulator_t>(hw_config, vspace);

    // further mandatory hw configuration (usually set by PHY layer)
    hw->set_nof_antennas(packet_sizes_maximum.tm_mode.N_TX);
    hw->set_samp_rate(dectnrp::phy::resampler_t::get_samp_rate_converted_with_temporary_overflow(
        dect_samp_rate_os, L, M));

    // we are interested in the signal shape at the correct sample rate
    const bool enforce_dectnrp_samp_rate_by_resampling = true;

    // hardware chose a sample rate, check if resampling is doable and with what parameters
    const auto resampler_param = dectnrp::phy::phy_config_t::get_resampler_param_verified(
        hw->get_samp_rate(), dect_samp_rate_os, enforce_dectnrp_samp_rate_by_resampling);

    // init IQ sample buffers
    hw->initialize_buffer_tx_pool(dectnrp::sp3::get_N_samples_in_packet_length_max(
        packet_sizes_maximum, hw->get_samp_rate()));

    auto tx = std::make_unique<dectnrp::phy::tx_t>(packet_sizes_maximum, os_min, resampler_param);

    // Network ID is a 32 bit integer which determines the scrambling sequence. These have to be
    // precalculated. Therefore its best, if the scrambling sequences are known a priori.
    const uint32_t network_id = randomgen.randi(0, UINT32_MAX - 1);
    tx->add_new_network_id(network_id);

    // add some more network IDs
    tx->add_new_network_id(network_id - 1);
    tx->add_new_network_id(network_id - 10);
    tx->add_new_network_id(network_id - 100);
    tx->add_new_network_id(network_id - 1000);

    const uint32_t PLCF_type = randomgen.randi(1, 2);

    // blocking call, will internally loop until valid packet size is found
    const auto packet_sizes_random =
        dectnrp::sp3::get_random_packet_sizes_within_rdc(radio_device_class_string, randomgen);

    // at this point, we now know that our packet can be generated

    // we will shift the entire spectrum in frequency domain
#ifdef APPLY_RANDOM_PHASE_AND_FREQUENCY_SHIFT_AT_TX
    const float oversampling = static_cast<double>(dect_samp_rate_os) /
                               static_cast<double>(packet_sizes_random.numerology.B_u_b_DFT);
    const float random_phase_rad = randomgen.rand_m1p1() * 2.0f * float(M_PI);
    // If oversampling >= 2.0, we can shift the entire spectrum by up to 50% of the bandwidth.
    // If oversampling = 1.0, we can at least shift within the 3 guards.
    const float random_cfo_Hz =
        randomgen.rand_m1p1() * (oversampling - 1.0) / 2.0 *
            static_cast<float>(packet_sizes_random.numerology.B_u_b_DFT) +
        randomgen.rand_m1p1() * static_cast<double>(packet_sizes_random.numerology.N_guards_top) *
            static_cast<float>(packet_sizes_random.numerology.delta_u_f);
#else
    const float random_phase_rad = 0.0f;
    const float random_cfo_Hz = 0.0f;
#endif

    // define additional PHY packet metadata
    const dectnrp::phy::tx_meta_t tx_meta = {
        .optimal_scaling_DAC = false,
        .DAC_scale = 1.0f,
        .iq_phase_rad = random_phase_rad,
        .iq_phase_increment_s2s_post_resampling_rad =
            dectnrp::common::adt::get_sample2sample_phase_inc(random_cfo_Hz, hw->get_samp_rate()),
        /* GI can never be of 0 length, there must be at least one zero sample. The minimum number
         * of samples in the GI is reached for critical sampling at the lowest sample rate possible.
         *      18.52us * 1.728MS/s = 32S
         *
         * Minimum percentage then is:
         *      32 * 3/100 = 0.96 < 1
         *      32 * 4/100 = 1.28 > 1
         */
        .GI_percentage = randomgen.randi(4, 100)};

    const uint32_t codebook_index =
        randomgen.randi(0,
                        dectnrp::sp3::W_t::get_codebook_index_max(
                            packet_sizes_random.tm_mode.N_TS, packet_sizes_random.tm_mode.N_TX));

    // define additional radio layer packet metadata
    const dectnrp::radio::buffer_tx_meta_t buffer_tx_meta = {
        .tx_order_id = 0, .tx_order_id_expect_next = -1, .tx_time_64 = 0, .busy_wait_us = 0};

    const uint32_t N_packets = 2;

    for (uint32_t p = 0; p < N_packets; ++p) {
        // make initial request for a HARQ buffer
        auto* hp_tx =
            hpp->get_process_tx(PLCF_type,
                                network_id,
                                packet_sizes_random.psdef,
                                dectnrp::phy::harq::finalize_tx_t::increase_rv_and_keep_running);

        // ID must be requested while process if fully locked
        const uint32_t hp_tx_id = hp_tx->get_id();

        dectnrp_assert(hp_tx != nullptr, "no HARQ buffer available");

        // write random PLCF data
        uint8_t* a_plcf = hp_tx->get_a_plcf();
        for (uint32_t i = 0; i < dectnrp::constants::plcf_type_2_byte; ++i) {
            a_plcf[i] = static_cast<uint8_t>(std::rand() % 256);
        }

        // write random TB data
        uint8_t* a_tb = hp_tx->get_a_tb();
        for (uint32_t i = 0; i < packet_sizes_random.N_TB_byte; ++i) {
            a_tb[i] = static_cast<uint8_t>(std::rand() % 256);
        }

        bool any_error = false;

        const uint32_t rep_max = dectnrp::constants::rv_max;

        for (uint32_t rep = 0; rep <= rep_max; ++rep) {
            // keep processing running
            if (rep > 0 && rep < rep_max) {
                hp_tx = hpp->get_process_tx_running(
                    hp_tx_id, dectnrp::phy::harq::finalize_tx_t::increase_rv_and_keep_running);
            }
            // terminate process after usage
            else if (rep == rep_max) {
                hp_tx = hpp->get_process_tx_running(
                    hp_tx_id, dectnrp::phy::harq::finalize_tx_t::reset_and_terminate);
            }

            const dectnrp::phy::tx_descriptor_t tx_descriptor(
                *hp_tx, codebook_index, tx_meta, buffer_tx_meta);

            auto* buffer_tx = hw->buffer_tx_pool->get_buffer_tx_to_fill();

            dectnrp_assert(buffer_tx != nullptr, "buffer unavailable");

            tx->generate_tx_packet(tx_descriptor, *buffer_tx);

            hw->set_all_buffers_as_transmitted();

            if (any_error) {
                break;
            }

            // reset the HARQ process or progress the rv
            hp_tx->finalize();
        }

        if (any_error) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::srand(time(NULL));

    const std::vector<std::string> rdc_vec = {
        "1.1.1.A", "8.1.1.A", "1.8.1.A", "2.8.2.A", "2.12.4.A", "8.12.8.A", "8.16.8.A"};

    const std::size_t N_runs = 2;

    for (std::size_t i = 0; i < N_runs; ++i) {
        for (auto rdc : rdc_vec) {
            if (generate_random_TX_packet_within_rdc_limits(rdc)) {
                dectnrp_print_wrn("packet generation test failed");
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}
