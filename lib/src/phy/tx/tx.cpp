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

#include "dectnrp/phy/tx/tx.hpp"

#include <volk/volk.h>

#include <algorithm>
#include <cmath>

extern "C" {
#include "srsran/phy/modem/mod.h"
#include "srsran/phy/utils/bit.h"
#include "srsran/phy/utils/vector.h"
#include "srsran/phy/utils/vector_simd.h"
}

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/phy/rx/rx_synced/rx_synced_param.hpp"
#include "dectnrp/sections_part3/fix/mod.hpp"

#ifdef PHY_TX_JSON_EXPORT
#include "dectnrp/common/json/json_export.hpp"
#include "dectnrp/constants.hpp"

// must be static in case TX is recreated for different packets
static std::atomic<uint32_t> json_file_cnt{0};
#endif

namespace dectnrp::phy {

tx_t::tx_t(const section3::packet_sizes_t maximum_packet_sizes_,
           const uint32_t os_min_,
           resampler_param_t resampler_param_)
    : tx_rx_t(maximum_packet_sizes_, os_min_, resampler_param_) {
    // srsran does not include 1024-QAM, unreasonable anyway, maximum MCS index is 9 (256-QAM at
    // R=5/6)
    srsran_modem_table_lte(&srsran_modem_table[0], SRSRAN_MOD_BPSK);
    srsran_modem_table_lte(&srsran_modem_table[1], SRSRAN_MOD_QPSK);
    srsran_modem_table_lte(&srsran_modem_table[2], SRSRAN_MOD_16QAM);
    srsran_modem_table_lte(&srsran_modem_table[3], SRSRAN_MOD_64QAM);
    srsran_modem_table_lte(&srsran_modem_table[4], SRSRAN_MOD_256QAM);
    // srsran_modem_table_lte(&srsran_modem_table[5], SRSRAN_MOD_1024QAM);

    for (uint32_t i = 0; i < srsran_modem_table.size(); ++i) {
        srsran_modem_table_bytes(&srsran_modem_table[i]);
    }

#ifdef PHY_TX_OFDM_WINDOWING
    dectnrp_assert(ofdm_vec.size() > 0, "no FFT size defined");

    windowing_vec.resize(ofdm_vec.size());

    for (uint32_t i = 0; i < windowing_vec.size(); ++i) {
        dectnrp_assert(fft_sizes_all[i] % constants::N_b_DFT_to_N_b_CP == 0,
                       "FFT size not a multiple of 8");

        dft::get_windowing(windowing_vec[i],
                           fft_sizes_all[i] / constants::N_b_DFT_to_N_b_CP,
                           PHY_TX_OFDM_WINDOWING);
    }
#endif

    const uint32_t u_max = maximum_packet_sizes.psdef.u;
    const uint32_t b_max = maximum_packet_sizes.psdef.b;
    const uint32_t b_idx_max = section3::phyres::b2b_idx[b_max];
    const uint32_t N_b_OCC_plus_DC_max = section3::phyres::N_b_OCC_plus_DC_lut[b_idx_max];
    const uint32_t N_b_DFT_os_max =
        dect_samp_rate_oversampled_max / constants::subcarrier_spacing_min_u_b;
    const uint32_t N_b_DFT_os_min =
        dect_samp_rate_oversampled_max / (constants::subcarrier_spacing_min_u_b * u_max);

    resampler = std::make_unique<resampler_t>(
        maximum_packet_sizes.tm_mode.N_TX,
        resampler_param.L,
        resampler_param.M,
        resampler_param_t::f_pass_norm[resampler_param_t::user_t::TX][os_min],
        resampler_param_t::f_stop_norm[resampler_param_t::user_t::TX][os_min],
        resampler_param_t::PASSBAND_RIPPLE_DONT_CARE,
        resampler_param_t::f_stop_att_dB[resampler_param_t::user_t::TX][os_min]);

    /* The resampler must be callable for every single OFDM symbol. Therefore, it cannot require
     * more than the minimum number of samples per OFDM symbol for one call.
     *
     * We always feed the resampler the IFFT result of one complete OFDM symbol in time domain. The
     * number of samples in such an OFDM symbol depends on the packet configuration, but can never
     * be smaller than N_b_DFT_os_min + 12.5% cyclic prefix (except for the last residual samples,
     * but those don't matter here).
     */
    dectnrp_assert(
        resampler->get_minimum_nof_input_samples() < (N_b_DFT_os_min + N_b_DFT_os_min / 8),
        "resampler needs more samples per call");

    for (uint32_t i = 0; i < maximum_packet_sizes.tm_mode.N_TS; ++i) {
        transmit_streams_stage.push_back(srsran_vec_cf_malloc(N_b_OCC_plus_DC_max));
    }

    beamforming_stage = srsran_vec_cf_malloc(N_b_OCC_plus_DC_max);

    for (uint32_t i = 0; i < maximum_packet_sizes.tm_mode.N_TX; ++i) {
        antenna_mapper_stage.push_back(srsran_vec_cf_malloc(N_b_DFT_os_max));
    }

    // see 6.3.6 in part 3
    const uint32_t N_samples_STF_CP_only_os_max =
        (u_max == 1) ? N_b_DFT_os_max * 3 / 4 : N_b_DFT_os_max * 5 / 4;

    for (uint32_t i = 0; i < maximum_packet_sizes.tm_mode.N_TX; ++i) {
#ifdef PHY_TX_OFDM_WINDOWING
        ifft_cp_stage.push_back(
            srsran_vec_cf_malloc(2 * N_samples_STF_CP_only_os_max + N_b_DFT_os_max));
#else
        ifft_cp_stage.push_back(
            srsran_vec_cf_malloc(N_samples_STF_CP_only_os_max + N_b_DFT_os_max));
#endif
    }

    antenna_ports.resize(maximum_packet_sizes.tm_mode.N_TX);
    antenna_ports_now.resize(maximum_packet_sizes.tm_mode.N_TX);
}

tx_t::~tx_t() {
    for (auto& elem : srsran_modem_table) {
        srsran_modem_table_free(&elem);
    }

#ifdef PHY_TX_OFDM_WINDOWING
    for (auto& elem : windowing_vec) {
        dft::free_windowing(elem);
    }
#endif

    for (auto& elem : transmit_streams_stage) {
        free(elem);
    }

    free(beamforming_stage);

    for (auto& elem : antenna_mapper_stage) {
        free(elem);
    }

    for (auto& elem : ifft_cp_stage) {
        free(elem);
    }
}

void tx_t::generate_tx_packet(const tx_descriptor_t& tx_descriptor_,
                              radio::buffer_tx_t& buffer_tx_) {
    // must be set before run_packet_dimensions()
    tx_descriptor = &tx_descriptor_;
    packet_sizes = &tx_descriptor_.hp_tx.get_packet_sizes();
    tx_meta = &tx_descriptor_.tx_meta;
    hb_plcf = tx_descriptor_.hp_tx.get_hb_plcf();
    hb_tb = tx_descriptor_.hp_tx.get_hb_tb();
    buffer_tx = &buffer_tx_;

    run_packet_dimensions();

    // set FEC configuration
    fec_cfg.PLCF_type = tx_descriptor->hp_tx.get_PLCF_type();
    fec_cfg.closed_loop = packet_sizes->tm_mode.cl;
    fec_cfg.beamforming = (tx_descriptor->codebook_index > 0) ? true : false;
    fec_cfg.N_TB_bits = packet_sizes->N_TB_bits;
    fec_cfg.N_bps = packet_sizes->mcs.N_bps;
    fec_cfg.rv = tx_descriptor->hp_tx.get_rv();
    fec_cfg.G = packet_sizes->G;
    fec_cfg.network_id = tx_descriptor->hp_tx.get_network_id();
    fec_cfg.Z = packet_sizes->psdef.Z;

    run_meta_dependencies();

    // antenna_ports points to front of all antennas
    buffer_tx->get_ant_streams(antenna_ports, N_samples_transmit_os_rs);

    // antenna_ports_now points to current samples of the antennas we use
    antenna_ports_now.resize(packet_sizes->tm_mode.N_TX);
    for (uint32_t i = 0; i < packet_sizes->tm_mode.N_TX; ++i) {
        antenna_ports_now[i] = antenna_ports[i];
    }

    fec->segmentate_and_pick_scrambling_sequence(fec_cfg);

    fec->encode_plcf(fec_cfg, *hb_plcf);

#ifndef PHY_TX_FEC_OF_PDC_CODEBLOCK_FOR_CODEBLOCK_OR_COMPLETE_AT_THE_BEGINNING
    // run FEC for complete TB
    fec->encode_tb(fec_cfg, *hb_tb);
#endif

#ifdef PHY_TX_BACKPRESSURED_OR_PACKET_IS_ALWAYS_COMPLETE
    // signal is backpressured, release immediately and start filling samples, radio will busy wait
    buffer_tx->set_transmittable(tx_descriptor->buffer_tx_meta);
#endif

    // create 98 complex QPSK symbols
    run_pcc_symbol_mapper_and_flipper();

    // flush for STF
    run_zero_stages();

    // load correct STF and copy into first transmit stream
    run_stf();

    // for STF, only the first transmit stream is non zero, therefore 1
    run_beamforming(1);

    // STF has a longer CP and a different scaling than any of the other OFDM symbols
    run_ifft_cp_scale(N_samples_STF_CP_only_os, final_scale_STF);

    // proposed in 2023 to improve performance in low SNR and high CFO regimes
    section3::stf_t::apply_cover_sequence(
        ifft_cp_stage,
        ifft_cp_stage,
        tm_mode.N_TX,
        packet_sizes->psdef.u,
        constants::N_samples_stf_pattern * packet_sizes->psdef.b * N_b_DFT_os / N_b_DFT);

    // length of STF CP must be known to resample the right amount of samples
    run_resampling_and_freq_shift(N_samples_STF_CP_only_os);

#ifdef PHY_TX_BACKPRESSURED_OR_PACKET_IS_ALWAYS_COMPLETE
    // notify radio layer of how many samples are now valid
    buffer_tx->set_tx_length_samples_cnt(index_sample_transmit_os_rs);
#endif

    // OFDM symbol by symbol loop excluding STF and GI
    for (ofdm_symb_idx = 1; ofdm_symb_idx <= packet_sizes->N_DF_symb; ++ofdm_symb_idx) {
        run_zero_stages();

        // assume one spatial stream for PCC, apply transmit diversity coding and copy into transmit
        // streams
        if (pcc.is_symbol_index(ofdm_symb_idx)) {
            run_pcc();
        }

        // copy DRS symbols into transmit streams
        if (drs.is_symbol_index(ofdm_symb_idx)) {
            run_drs();
        }

        // If transmit diversity coding is used, we have only one spatial stream. Apply transmit
        // diversity coding and copy into transmit streams. For any other mode, apply stream
        // demultiplexing (trivial if SISO) and copy spatial streams directly into transmit streams.
        if (pdc.is_symbol_index(ofdm_symb_idx)) {
            run_pdc();
        }

        // transmit streams are filled with PCC, DRS and PDC, so now beamform entire OFDM symbol
        run_beamforming(tm_mode.N_TS);

        // scale in frequency domain, execute IFFT and insert CP
        run_ifft_cp_scale(N_b_CP_os, final_scale);

        // resample from DECTNRP sample rate to actual hardware sample rate and write into output
        // buffer
        run_resampling_and_freq_shift(N_b_CP_os);

#ifdef PHY_TX_BACKPRESSURED_OR_PACKET_IS_ALWAYS_COMPLETE
        // notify radio layer of how many samples are now valid
        buffer_tx->set_tx_length_samples_cnt(index_sample_transmit_os_rs);
#endif
    }

    // flush internal resampler buffers
    run_residual_resampling();

    // we zero GI, can be partial GI
    run_GI();

#ifdef PHY_TX_JSON_EXPORT
    write_all_data_to_json();
#endif

    // tell tx buffer that all samples were written
    buffer_tx->set_tx_length_samples_cnt(N_samples_transmit_os_rs);

#ifndef PHY_TX_BACKPRESSURED_OR_PACKET_IS_ALWAYS_COMPLETE
    // notify radio layer that TX buffer is filled and ready to go
    buffer_tx->set_transmittable(tx_descriptor->buffer_tx_meta);
#endif

    // run some check after packet processing
    dectnrp_assert(PCC_idx == constants::pcc_cells, "incorrect number of PCC subcarriers written");
    dectnrp_assert(DRS_idx == packet_sizes->N_DRS_subc,
                   "incorrect number of DRS subcarriers written");
    dectnrp_assert(fec->get_wp() == packet_sizes->G,
                   "incorrect position of FEC write pointer bits written");
    dectnrp_assert(PDC_bits_idx == packet_sizes->G, "incorrect number of PDC bits written");
    dectnrp_assert(PDC_subc_idx == packet_sizes->N_PDC_subc * tm_mode.N_SS,
                   "incorrect number of PDC subcarriers written across all transmit streams");
    dectnrp_assert(PDC_nof_cmplx_subc_residual == 0,
                   "incorrect number of complex PDC symbols left and PDC_nof_cmplx_subc_residual");
    dectnrp_assert(index_sample_no_GI_os == N_samples_packet_no_GI_os,
                   "incorrect number of oversampled IQ samples for packet without GI written");
    dectnrp_assert(
        index_sample_transmit_os_rs == N_samples_transmit_os_rs,
        "incorrect number of oversampled and resampled IQ samples for packet with GI written");
}

#ifdef PHY_TX_JSON_EXPORT
void tx_t::write_all_data_to_json() const {
    // create filename with unique file count
    std::ostringstream filename;
    filename << "tx_packet_" << std::setw(10) << std::setfill('0') << json_file_cnt.load();

    // increase static file count
    json_file_cnt.fetch_add(1);

    nlohmann::json j_packet_data;

    j_packet_data["u"] = packet_sizes->psdef.u;
    j_packet_data["b"] = packet_sizes->psdef.b;
    j_packet_data["PacketLengthType"] = packet_sizes->psdef.PacketLengthType;
    j_packet_data["PacketLength"] = packet_sizes->psdef.PacketLength;
    j_packet_data["tm_mode"] = packet_sizes->psdef.tm_mode_index;
    j_packet_data["mcs_index"] = packet_sizes->psdef.mcs_index;
    j_packet_data["Z"] = packet_sizes->psdef.Z;
    j_packet_data["oversampling"] = oversampling;
    j_packet_data["codebook_index"] = tx_descriptor->codebook_index;
    j_packet_data["PLCF_type"] = fec_cfg.PLCF_type;
    j_packet_data["rv"] = fec_cfg.rv;
    j_packet_data["network_id"] = fec_cfg.network_id;
    j_packet_data["N_samples_packet_no_GI_os_rs"] = N_samples_packet_no_GI_os_rs;
    j_packet_data["N_samples_transmit_os_rs"] = N_samples_transmit_os_rs;

    // save additional meta data
    j_packet_data["tx_descriptor"]["tx_order_id"] = tx_descriptor->buffer_tx_meta.tx_order_id;
    j_packet_data["tx_descriptor"]["tx_time_64"] = tx_descriptor->buffer_tx_meta.tx_time_64;

    j_packet_data["tx_meta"]["iq_phase_rad"] = tx_meta->iq_phase_rad;
    j_packet_data["tx_meta"]["iq_phase_increment_s2s_post_resampling_rad"] =
        tx_meta->iq_phase_increment_s2s_post_resampling_rad;
    j_packet_data["tx_meta"]["GI_percentage"] = tx_meta->GI_percentage;

    // convert packed bits to unpacked vector of integers
    auto unpack_into_vector = [](const uint8_t* bits_packed, const uint32_t N_bits) {
        // stack allocation can fail for very large packets
        uint8_t* bits_unpacked = new uint8_t[N_bits];

        srsran_bit_unpack_vector(bits_packed, bits_unpacked, N_bits);

        // write into vector
        std::vector<uint32_t> bits_unpacked_vec(N_bits, 0);
        for (uint32_t i = 0; i < N_bits; ++i) {
            bits_unpacked_vec[i] = static_cast<uint32_t>(bits_unpacked[i]);
        }

        delete[] bits_unpacked;

        return bits_unpacked_vec;
    };

    // PLCF before coding, PCC after coding
    auto PLCF_vec = unpack_into_vector(
        hb_plcf->get_a(),
        (fec_cfg.PLCF_type == 1) ? constants::plcf_type_1_bit : constants::plcf_type_2_bit);
    auto PCC_vec = unpack_into_vector(hb_plcf->get_d(), constants::pcc_bits);

    // TB before coding, PDC after coding
    auto TB_vec = unpack_into_vector(hb_tb->get_a(), packet_sizes->N_TB_bits);
    auto PDC_vec = unpack_into_vector(hb_tb->get_d(), packet_sizes->G);

    j_packet_data["data"]["binary"]["PLCF"] = PLCF_vec;
    j_packet_data["data"]["binary"]["PCC"] = PCC_vec;
    j_packet_data["data"]["binary"]["TB"] = TB_vec;
    j_packet_data["data"]["binary"]["PDC"] = PDC_vec;

    j_packet_data["resampling"]["samp_rate"] = resampler_param.hw_samp_rate;
    j_packet_data["resampling"]["L"] = resampler_param.L;
    j_packet_data["resampling"]["M"] = resampler_param.M;

    j_packet_data["resampling"]["f_pass_norm"] = resampler->f_pass_norm;
    j_packet_data["resampling"]["f_stop_norm"] = resampler->f_stop_norm;
    j_packet_data["resampling"]["passband_ripple_dB"] = resampler->passband_ripple_dB;
    j_packet_data["resampling"]["stopband_attenuation_dB"] = resampler->stopband_attenuation_dB;
    j_packet_data["resampling"]["oversampling_minimum"] = os_min;

    /* We concatenate the antenna streams into two vectors:
     *
     * I_vec = [real of all samples from antenna stream 0
     *          real of all samples from antenna stream 1
     *          ...
     *          real of all samples from antenna stream N_TX-1]
     * Q_vec = [imag of all samples from antenna stream 0
     *          imag of all samples from antenna stream 1
     *          ...
     *          imag of all samples from antenna stream N_TX-1]
     */
    std::vector<float> I_vec(N_samples_transmit_os_rs * tm_mode.N_TX, 0.0f);
    std::vector<float> Q_vec(N_samples_transmit_os_rs * tm_mode.N_TX, 0.0f);
    uint32_t idx = 0;
    for (uint32_t ant_idx = 0; ant_idx < tm_mode.N_TX; ++ant_idx) {
        // get pointer to front of buffer
        std::vector<cf_t*> IQ_vec(buffer_tx->nof_antennas);
        buffer_tx->get_ant_streams(IQ_vec, N_samples_transmit_os_rs);

        // save all real and all imag values of this antenna
        for (uint32_t j = 0; j < N_samples_transmit_os_rs; ++j) {
            I_vec[idx] = __real__(IQ_vec[ant_idx][j]);
            Q_vec[idx] = __imag__(IQ_vec[ant_idx][j]);
            ++idx;
        }
    }

    j_packet_data["data"]["IQ"]["real"] = I_vec;
    j_packet_data["data"]["IQ"]["imag"] = Q_vec;

    common::json_export_t::write_to_disk(j_packet_data, filename.str());
}
#endif

void tx_t::run_packet_dimensions() {
    dectnrp_assert(packet_sizes->psdef.Z == maximum_packet_sizes.psdef.Z,
                   "Z not the same as in RDC");

    dectnrp_assert(packet_sizes->psdef.u <= maximum_packet_sizes.psdef.u, "Mu too large");

    dectnrp_assert(dect_samp_rate_oversampled_max % packet_sizes->numerology.delta_u_f == 0,
                   "nominal bandwidth not a multiple of delta_u_f");

    // how many subcarriers does the nominal, oversampled bandwidth contain?
    N_b_DFT_os = dect_samp_rate_oversampled_max / packet_sizes->numerology.delta_u_f;

    // set IFFT size
    set_ofdm_vec_idx_effective(N_b_DFT_os);

    dectnrp_assert(packet_sizes->psdef.b <= maximum_packet_sizes.psdef.b, "Beta too large");

    N_b_DFT = packet_sizes->numerology.N_b_DFT;
    N_b_OCC = packet_sizes->numerology.N_b_OCC;
    N_b_OCC_plus_DC = N_b_OCC + 1;

    dectnrp_assert((N_b_DFT_os - N_b_DFT) % 2 == 0,
                   "nof guard carriers due to oversampling not divisable by 2");

    set_N_subc_offset_lower_half_os(N_b_DFT_os, N_b_DFT, packet_sizes->numerology.N_guards_bottom);

#ifdef PHY_TX_JSON_EXPORT
    // can be 1.3333 when b_max=16 and b=12, can be 1.5 when b_max=12 and b=8
    oversampling = static_cast<double>(N_b_DFT_os) / static_cast<double>(N_b_DFT);
#endif

    dectnrp_assert((packet_sizes->N_samples_STF_CP_only * N_b_DFT_os) % N_b_DFT == 0,
                   "oversampling does not lead to integer length of STF CP");

    dectnrp_assert((packet_sizes->numerology.N_b_CP * N_b_DFT_os) % N_b_DFT == 0,
                   "oversampling does not lead to integer length of CP");

    N_samples_STF_CP_only_os = packet_sizes->N_samples_STF_CP_only * N_b_DFT_os / N_b_DFT;
    N_b_CP_os = packet_sizes->numerology.N_b_CP * N_b_DFT_os / N_b_DFT;

    dectnrp_assert(packet_sizes->tm_mode.N_TX <= maximum_packet_sizes.tm_mode.N_TX,
                   "nof antennas required larger than nof physical antennas");

    tm_mode = packet_sizes->tm_mode;

    reconfigure_packet_components(packet_sizes->psdef.b, tm_mode.N_TS);

    // see table 7.2-1 in part 3
    transmit_diversity_mode =
        (tm_mode.index == 1 || tm_mode.index == 5 || tm_mode.index == 10) ? true : false;

    dectnrp_assert(packet_sizes->psdef.mcs_index <= maximum_packet_sizes.mcs.index,
                   "MCS larger than allowed");

    dectnrp_assert(packet_sizes->psdef.mcs_index <= limits::dectnrp_max_mcs_index,
                   "unable to set modem table for MCS index");

    // set symbol mapper
    switch (packet_sizes->psdef.mcs_index) {
        // BPSK
        case 0:
            srsran_modem_table_effective = &srsran_modem_table[0];
            break;
        // QPSK
        case 1:
        case 2:
            srsran_modem_table_effective = &srsran_modem_table[1];
            break;
        // 16 QAM
        case 3:
        case 4:
            srsran_modem_table_effective = &srsran_modem_table[2];
            break;
        // 64 QAM
        case 5:
        case 6:
        case 7:
            srsran_modem_table_effective = &srsran_modem_table[3];
            break;
        // 256 QAM
        case 8:
        case 9:
        default:
            srsran_modem_table_effective = &srsran_modem_table[4];
    }

    reset_common_counters();

    index_sample_no_GI_os = 0;
    index_sample_transmit_os_rs = 0;
}

void tx_t::run_meta_dependencies() {
    dectnrp_assert((packet_sizes->N_samples_packet_no_GI * N_b_DFT_os) % N_b_DFT == 0,
                   "oversampling does not lead to integer length of packet without GI");

    // packet length without GI after oversampling
    N_samples_packet_no_GI_os = packet_sizes->N_samples_packet_no_GI * N_b_DFT_os / N_b_DFT;

    // packet length without GI after oversampling and resampling
    N_samples_packet_no_GI_os_rs = common::adt::ceil_divide_integer(
        N_samples_packet_no_GI_os * resampler_param.L, resampler_param.M);

    dectnrp_assert(N_samples_packet_no_GI_os_rs <=
                       resampler->get_N_samples_after_resampling(N_samples_packet_no_GI_os),
                   "resamples creates less samples than required for packet without GI");

    // packet length with GI after oversampling
    const uint32_t N_samples_packet_os = packet_sizes->N_samples_packet * N_b_DFT_os / N_b_DFT;

    dectnrp_assert(N_samples_packet_os % resampler_param.M == 0,
                   "oversampled packet length with GI not a multiple of M");

    // packet length with GI after oversampling and resampling
    const uint32_t N_samples_packet_os_rs =
        (N_samples_packet_os / resampler_param.M) * resampler_param.L;

    dectnrp_assert(N_samples_packet_no_GI_os_rs < N_samples_packet_os_rs,
                   "packet with GI must be longer than without");

    const uint32_t GI_samples_os_rs = N_samples_packet_os_rs - N_samples_packet_no_GI_os_rs;

    dectnrp_assert(std::abs(static_cast<double>(N_samples_packet_os - N_samples_packet_no_GI_os) *
                                static_cast<double>(resampler_param.L) /
                                static_cast<double>(resampler_param.M) -
                            static_cast<double>(GI_samples_os_rs)) <= 1.0,
                   "GI before and after resampling deviates more than one sample");

    dectnrp_assert(tx_meta->GI_percentage <= 100, "GI percentage too large");

    // how many samples do we ultimately transmit?
    N_samples_transmit_os_rs =
        N_samples_packet_no_GI_os_rs + GI_samples_os_rs * tx_meta->GI_percentage / 100;

    dectnrp_assert(
        N_samples_packet_no_GI_os_rs < N_samples_transmit_os_rs,
        "at least one zero sample should be transmitted as GI to force DAC + PA output to zero");

    dectnrp_assert(N_samples_transmit_os_rs <= N_samples_packet_os_rs,
                   "transmission length too long");

    dectnrp_assert(N_samples_packet_os_rs <= buffer_tx->ant_streams_length_samples,
                   "packet length with GI too long");

    dectnrp_assert(tm_mode.N_TX <= buffer_tx->nof_antennas, "nof antennas larger than buffer_tx");

    dectnrp_assert(tx_descriptor->codebook_index <=
                       section3::W_t::get_codebook_index_max(tm_mode.N_TS, tm_mode.N_TX),
                   "codebook index out of bound");

    dectnrp_assert((tx_descriptor->codebook_index > 0 && fec_cfg.beamforming) ||
                       (tx_descriptor->codebook_index == 0 && !fec_cfg.beamforming),
                   "codebook index >0, but FEC configuration states beamforming is deactivated");

    // amplitude/power scaling
    float scale_common = tx_meta->DAC_scale;
    if (!tx_meta->optimal_scaling_DAC) {
        // as described in standard
        scale_common *=
            W.get_scaling_factor(tm_mode.N_TS, tm_mode.N_TX, tx_descriptor->codebook_index);
    } else {
        // optimized for dynamic range
        scale_common *= W.get_scaling_factor_optimal_DAC(
            tm_mode.N_TS, tm_mode.N_TX, tx_descriptor->codebook_index);
    }
    // see 6.3.6 in part 3
    final_scale_STF = 1.0f / std::sqrt(float(N_b_OCC / 4)) * scale_common;
    final_scale = 1.0f / std::sqrt(float(N_b_OCC)) * scale_common;

    resampler->reset();

    mixer.set_phase(tx_meta->iq_phase_rad);
    mixer.set_phase_increment(tx_meta->iq_phase_increment_s2s_post_resampling_rad);
}

void tx_t::run_pcc_symbol_mapper_and_flipper() {
    // PCC always has 196 bits after channel coding, the number of complex QPSK symbols will always
    // be 196/2=98. This function returns the number of symbols written.
    if (section3::fix::srsran_mod_modulate_bytes_FIX(
            &srsran_modem_table[1], hb_plcf->get_d(), pcc_qpsk_symbols, constants::pcc_bits) !=
        constants::pcc_cells) {
        dectnrp_assert_failure("incorrect number of PCC symbols");
    }

    /* Example for Transmit Diversity Precoding with 4 Transmit Streams:
     *
     * TS:      3   2   1   0
     *
     *          .   .   .   .
     *          .   .   .   .
     *          .   .   .   .
     *          s6** x    s7   x
     * i=3      s7*  x    s6   x
     *          x    s4** x    s5
     * i=2      x    s5*  x    s4
     *          s2** s3   x    x
     * i=1      s3*  s2   x    x
     *          x    x    s0** s1
     * i=0      x    x    s1*  s0
     *
     * Symbols marked with  * have -real  imag pattern applied (equivalent to -()*).
     * Symbols marked with ** have  real -imag pattern applied (equivalent to ()*).
     *
     * Alamouti demonstrates a space-time code, we utilize a space-frequency code. The channel is
     * assumed to be constant for two neighbouring subcarriers.
     * https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=730453
     *
     * Receiver processing:
     *
     * Note: The operator * stands for conjugate, not multiplication.
     * Note: We ignore AWGN.
     *
     * r0 = h0 s0 - h1 s1*
     * r1 = h0 s1 + h1 s0*
     *
     * s0_estim = h0* r0                  + h1 r1*
     *          = h0* [h0 s0 - h1 s1*]    + h1 [h0 s1 + h1 s0*]*
     *          = h0* [h0 s0 - h1 s1*]    + h1 [h0* s1* + h1* s0]
     *          = |h0|^2 s0 - h0* h1 s1*  + h0* h1 s1* + |h1|^2 s0
     *          = (|h0|^2 + |h1|^2) s0
     *
     * s1_estim = -h1 r0*                 + h0* r1
     *          = -h1 [h0 s0 - h1 s1*]*   + h0* [h0 s1 + h1 s0*]
     *          = -h1 [h0* s0* - h1* s1]  + h0* [h0 s1 + h1 s0*]
     *          = -h0* h1 s0* + |h1|^2 s1 + |h0|^2 s1 + h0* h1 s0*
     *          = (|h0|^2 + |h1|^2) s1
     */
    if (tm_mode.N_TS > 1) {
        // We have 98 complex symbols with index 0 1 2 3 4 5 6 7 8 9 ... 96 97.
        // We create a copy with pairwise flipped indices, i.e. 1 0 3 2 5 4 7 6 9 8 ... 97 96
        for (uint32_t i = 0; i < constants::pcc_cells; i += 2) {
            pcc_qpsk_symbols_flipped[i] = pcc_qpsk_symbols[i + 1];
            pcc_qpsk_symbols_flipped[i + 1] = pcc_qpsk_symbols[i];
        }

        // change the pattern from   real imag   real imag ... to -real imag   real -imag ...
#ifdef PHY_TX_PCC_FLIPPING_WITH_SIMD
        // *2 for using float function (fff) instead of complex
        srsran_vec_prod_fff((float*)pcc_qpsk_symbols_flipped,
                            (float*)Y_i->get_pattern(),
                            (float*)pcc_qpsk_symbols_flipped,
                            constants::pcc_cells * 2);
#else
        float* real_imag_inversion = (float*)pcc_qpsk_symbols_flipped;
        for (uint32_t i = 0; i < constants::pcc_bits; i += 4) {
            real_imag_inversion[i] = -real_imag_inversion[i];
            real_imag_inversion[i + 3] = -real_imag_inversion[i + 3];
        }
#endif
    }
}

void tx_t::run_residual_resampling() {
    // resample final samples
    uint32_t N_new_output_samples = resampler->resample_final_samples(antenna_ports_now);

    // apply frequency shift
    mixer.mix_phase_continuous(antenna_ports_now, antenna_ports_now, N_new_output_samples);

    // zero same amount of samples in unused antenna streams
    for (uint32_t i = tm_mode.N_TX; i < antenna_ports.size(); ++i) {
        srsran_vec_cf_zero(&antenna_ports[i][index_sample_transmit_os_rs], N_new_output_samples);
    }

    dectnrp_assert(
        N_samples_packet_no_GI_os_rs <= index_sample_transmit_os_rs + N_new_output_samples,
        "not enough samples resampled");

    dectnrp_assert(
        index_sample_transmit_os_rs + N_new_output_samples <= buffer_tx->ant_streams_length_samples,
        "too many samples resampled");

    /* At this point, we have resampled the final samples of the resampler and written them to the
     * output buffer. We also made sure we wrote enought samples. We can savely overwrite the value
     * of index_sample_transmit_os_rs.
     */
    index_sample_transmit_os_rs = N_samples_packet_no_GI_os_rs;
}

void tx_t::run_GI() {
    // zero the GI across all antennas
    for (uint32_t i = 0; i < antenna_ports.size(); ++i) {
        srsran_vec_cf_zero(&antenna_ports[i][index_sample_transmit_os_rs],
                           N_samples_transmit_os_rs - N_samples_packet_no_GI_os_rs);
    }

    index_sample_transmit_os_rs += N_samples_transmit_os_rs - N_samples_packet_no_GI_os_rs;
}

void tx_t::run_zero_stages() {
    // zero only transmit streams in use
    for (uint32_t i = 0; i < tm_mode.N_TS; ++i) {
        srsran_vec_cf_zero(transmit_streams_stage[i], N_b_OCC_plus_DC);
    }

    srsran_vec_cf_zero(beamforming_stage, N_b_OCC_plus_DC);

    for (uint32_t i = 0; i < tm_mode.N_TX; ++i) {
        srsran_vec_cf_zero(antenna_mapper_stage[i], N_b_DFT_os);
    }
}

void tx_t::run_beamforming(const uint32_t N_TS_non_zero) {
    /* How does beamforming work?
     *
     * 1) Request current beamforming matrix W_mat.
     * 2) For instance, assume W_mat has the following structure:
     *
     *      N_TX=4 and N_TS=2, therefore 4x2 matrix given as 1d-vector
     *
     *          W_mat = (A, B, C, D, E, F, G, H)
     *
     *          antenna_mapper_stage0         A B
     *          antenna_mapper_stage1    =    C D   *     TS0
     *          antenna_mapper_stage2         E F         TS1
     *          antenna_mapper_stage3         G H
     *
     * 3) Pick next TS, at first TS0.
     * 4) Pick respective column, at first A, C, E, G, then B, D, F, H.
     * 5) Pick next column value, at first A.
     *
     *      Check if
     *
     *      A == 0. If so, do nothing and go to 5).
     *
     *      A == 1. If so, do src = TS0 for a simple memcpy.
     *
     *      A != 1. If so, do beamforming_stage = A*TS0 and src=beamforming_stage.
     *
     *      Check if A, C, E or G are the same as A. This way we can reuse the result of the
     *      multiplication A*TS0. If so, superimpose antenna_mapper_stage0/1/2/3 += src.
     *
     * 6) Go to 5).
     *
     * 7) Got to 3).
     */

    // request beamforming matrix
    const auto& W_mat = W.get_W(tm_mode.N_TS, tm_mode.N_TX, tx_descriptor->codebook_index);

    // transmit streams
    for (uint32_t i = 0; i < N_TS_non_zero; ++i) {
        /* Which index in W_mat are we currently processing? In the above description, the matrix
         * entries corresponds to the following indices:
         *
         *      (0, 1, 2, 3, 4, 5, 6, 7) = (A, B, C, D, E, F, G, H).
         */
        uint32_t W_mat_idx = i;

        // one bit for each antenna stream that received a weighted copy of the TS with index i
        uint32_t mask = 0;

        // antenna streams
        for (uint32_t j = 0; j < tm_mode.N_TX; ++j) {
            // if this antennas stream has been processed, go to next one
            if (((mask >> j) & 1) == 1) {
                W_mat_idx += tm_mode.N_TS;
                continue;
            }

            const cf_t weight = W_mat[W_mat_idx];

            // skip if zero weight
            if (__real__(weight) == 0.0f && __imag__(weight) == 0.0f) {
                W_mat_idx += tm_mode.N_TS;
                mask |= 1 << j;
                continue;
            }

            // if 1.0 directly copy from TS instead of first copying onto the beamforming stage
            const cf_t* src = nullptr;
            if (__real__(weight) == 1.0f && __imag__(weight) == 0.0f) {
                src = transmit_streams_stage[i];
            } else {
                // first multiply with weight onto beamforming stage
                srsran_vec_sc_prod_ccc_simd(
                    transmit_streams_stage[i], weight, beamforming_stage, N_b_OCC_plus_DC);

                src = beamforming_stage;
            }

            /* At this point, one of the TS has been multiplied with a weight and is ready to be
             * added to the respective antenna stream. However, other antenna streams may also have
             * the same weight for the given TS. Therefore, we go over all the next antennas streams
             * and check whether they are using the same weight.
             */

            // index of weight we compare
            uint32_t W_mat_idx_compare = W_mat_idx;

            // next antenna streams
            for (uint32_t j_compare = j; j_compare < tm_mode.N_TX; ++j_compare) {
                // if this antennas stream has been processed, go to next one
                if (((mask >> j) & 1) == 1) {
                    W_mat_idx_compare += tm_mode.N_TS;
                    continue;
                }

                // multiplication reusable?
                if (weight == W_mat[W_mat_idx_compare]) {
                    // our FFT stage requires a mirrored spectrum

                    /* Copy upper subcarriers. Note that complex float are used with a kernel for
                     * floats: N_b_OCC + 2 = (N_b_OCC_1_2 + 1) * 2 = 2*N_b_OCC_1_2 + 1 + 1
                     */
                    srsran_vec_add_fff_simd((const float*)antenna_mapper_stage[j_compare],
                                            (const float*)&src[N_b_OCC / 2],
                                            (float*)antenna_mapper_stage[j_compare],
                                            N_b_OCC + 2);

                    /* Copy lower occupied. Note that complex float are used with a kernel for
                     * floats: N_b_OCC = N_b_OCC_1_2 * 2
                     */
                    srsran_vec_add_fff_simd(
                        (const float*)&antenna_mapper_stage[j_compare][N_subc_offset_lower_half_os],
                        (const float*)src,
                        (float*)&antenna_mapper_stage[j_compare][N_subc_offset_lower_half_os],
                        N_b_OCC);

                    mask |= 1 << j_compare;
                }

                // skip other TS
                W_mat_idx_compare += tm_mode.N_TS;
            }

            // skip other TS
            W_mat_idx += tm_mode.N_TS;
        }

        dectnrp_assert(mask == static_cast<uint32_t>((1 << tm_mode.N_TX) - 1),
                       "not all antennas streams processed");
    }
}

void tx_t::run_ifft_cp_scale(const uint32_t N_samples_in_CP_os, float scale) {
    // scale in frequency domain so we don't have to scale the cyclic prefix in time domain
    for (uint32_t i = 0; i < tm_mode.N_TX; ++i) {
        srsran_vec_sc_prod_cfc_simd(
            antenna_mapper_stage[i], scale, antenna_mapper_stage[i], N_b_OCC / 2 + 1);
        srsran_vec_sc_prod_cfc_simd(&antenna_mapper_stage[i][N_subc_offset_lower_half_os],
                                    scale,
                                    &antenna_mapper_stage[i][N_subc_offset_lower_half_os],
                                    N_b_OCC / 2);
    }

    // transform antenna stream into time domain and add cyclic prefix
    for (uint32_t i = 0; i < tm_mode.N_TX; ++i) {
        dft::single_symbol_tx_ofdm_zero_copy(ofdm_vec[ofdm_vec_idx_effective],
                                             antenna_mapper_stage[i],
                                             ifft_cp_stage[i],
                                             N_samples_in_CP_os);
    }

#ifdef PHY_TX_OFDM_WINDOWING
    for (uint32_t i = 0; i < tm_mode.N_TX; ++i) {
        // If index_sample_no_GI_os == 0, we are at the first OFDM symbol and we don't apply
        // windowing to the cyclic prefix.
        if (index_sample_no_GI_os > 0) {
            // apply rising edge cosine window to cyclic prefix
            volk_32fc_32f_multiply_32fc(
                (lv_32fc_t*)ifft_cp_stage[i],
                (const lv_32fc_t*)ifft_cp_stage[i],
                (const float*)windowing_vec[ofdm_vec_idx_effective].raised_cosine,
                windowing_vec[ofdm_vec_idx_effective].length);

            // overlap falling edge of former OFDM symbol and overlap with cylic prefix
            volk_32fc_x2_add_32fc(
                (lv_32fc_t*)ifft_cp_stage[i],
                (const lv_32fc_t*)ifft_cp_stage[i],
                (const lv_32fc_t*)&ifft_cp_stage[i][N_samples_in_CP_os + N_b_DFT_os],
                windowing_vec[ofdm_vec_idx_effective].length);
        }

        // apply falling edge to front of current OFDM symbol and write to stage for later use
        volk_32fc_32f_multiply_32fc(
            (lv_32fc_t*)&ifft_cp_stage[i][N_samples_in_CP_os + N_b_DFT_os],
            (const lv_32fc_t*)&ifft_cp_stage[i][N_samples_in_CP_os],
            (const float*)windowing_vec[ofdm_vec_idx_effective].raised_cosine_inv,
            windowing_vec[ofdm_vec_idx_effective].length);
    }
#endif

    index_sample_no_GI_os += N_samples_in_CP_os + N_b_DFT_os;
}

void tx_t::run_resampling_and_freq_shift(const uint32_t N_samples_in_CP_os) {
    // resample only the lower antenna ports we are using
    uint32_t N_new_output_samples =
        resampler->resample(std::vector<const cf_t*>(ifft_cp_stage.begin(), ifft_cp_stage.end()),
                            antenna_ports_now,
                            N_samples_in_CP_os + N_b_DFT_os);

    // apply frequency shift
    mixer.mix_phase_continuous(antenna_ports_now, antenna_ports_now, N_new_output_samples);

    // zero same amount of samples on the upper antenna ports that we are not using
    for (uint32_t i = tm_mode.N_TX; i < antenna_ports.size(); ++i) {
        srsran_vec_cf_zero(&antenna_ports[i][index_sample_transmit_os_rs], N_new_output_samples);
    }

    index_sample_transmit_os_rs += N_new_output_samples;

    // create vector pointing to current sample index
    for (uint32_t i = 0; i < tm_mode.N_TX; ++i) {
        antenna_ports_now[i] = &antenna_ports[i][index_sample_transmit_os_rs];
    }
}

void tx_t::run_stf() {
    // load reference to prefilled STF symbol in frequency domain
    const std::vector<cf_t>& s = stf.get_stf(packet_sizes->psdef.b, tm_mode.N_eff_TX);

    // STF has to be copied for memory-alignment
    srsran_vec_cf_copy(transmit_streams_stage[0], &s[0], N_b_OCC_plus_DC);
}

void tx_t::run_pcc() {
    const std::vector<uint32_t>& k_i_one_symbol = pcc.get_k_i_one_symbol();

    // PCC always uses transmit diversity coding, unless it is SISO
    if (tm_mode.N_TS == 1) {
        // write complex QPSK symbols directly into first transmit stream at correct subcarrier
        for (uint32_t i : k_i_one_symbol) {
            transmit_streams_stage[0][i] = pcc_qpsk_symbols[PCC_idx++];
        }
    } else {
        // which transmit stream is used for two consecutive complex symbols?
        const common::vec2d<uint32_t>& index_mat_N_TS_x = Y_i->get_index_mat_N_TS_x(tm_mode.N_TS);

        // modulo operator
        const uint32_t modulo = Y_i->get_modulo(tm_mode.N_TS);

        // copy into correct transmit stream at correct subcarrier
        uint32_t i_mod_x, TS_index_A, TS_index_B;
        for (uint32_t k_i : k_i_one_symbol) {
            // PCC_idx/2 = i
            i_mod_x = (PCC_idx / 2) % modulo;

            TS_index_A = index_mat_N_TS_x[i_mod_x][0];
            TS_index_B = index_mat_N_TS_x[i_mod_x][1];

            transmit_streams_stage[TS_index_A][k_i] = pcc_qpsk_symbols[PCC_idx];
            transmit_streams_stage[TS_index_B][k_i] = pcc_qpsk_symbols_flipped[PCC_idx];

            ++PCC_idx;
        }
    }
}

void tx_t::run_drs() {
    // k_i are the indices, y_DRS_i are the complex values
    const common::vec2d<uint32_t>& k_i = drs.get_k_i();
    const common::vec2d<cf_t>& y_DRS_i = drs.get_y_DRS_i();

    // load indices of transmit streams in this symbol
    const uint32_t TS_idx_first = drs.get_ts_idx_first();
    const uint32_t TS_idx_last = drs.get_ts_idx_last_and_advance();

    // number of DRS cells
    const uint32_t k_i_size = k_i[0].size();

    // count for final assert
    DRS_idx += k_i_size * (TS_idx_last - TS_idx_first + 1);

    // fill transmit streams
    for (uint32_t TS_idx = TS_idx_first; TS_idx <= TS_idx_last; ++TS_idx) {
        cf_t* ts = transmit_streams_stage[TS_idx];
        const std::vector<uint32_t>& k_i_ = k_i[TS_idx % 4];
        const std::vector<cf_t>& y_DRS_i_ = y_DRS_i[TS_idx % 4];

        for (uint32_t i = 0; i < k_i_size; ++i) {
            ts[k_i_[i]] = y_DRS_i_[i];
        }
    }
}

void tx_t::run_pdc() {
    const std::vector<uint32_t>& k_i_one_symbol = pdc.get_k_i_one_symbol();

    const uint32_t k_i_one_symbol_size = k_i_one_symbol.size();

    // How many complex subcarrier do we need for this symbol?
    const uint32_t nof_cmplx_subc = tm_mode.N_SS * k_i_one_symbol_size;

    // Maybe we have some old subc left, so how many new ones do we need?
    const uint32_t nof_cmlpx_subc_new = nof_cmplx_subc - PDC_nof_cmplx_subc_residual;

    // How many new bits do we have to process to fill those complex subcarriers?
    const uint32_t nof_bits_new = nof_cmlpx_subc_new * packet_sizes->mcs.N_bps;

    // we process in multiples of 24 bits to make sure we process only full bytes and multiples of
    // N_bps
    const uint32_t nof_bits_new_24 = common::adt::ceil_divide_integer(nof_bits_new, 24U) * 24U;

    // how many bits are still processable? this is relevant for the last OFDM symbol in a packet
    const uint32_t nof_bits_processable = std::min(nof_bits_new_24, packet_sizes->G - PDC_bits_idx);

    dectnrp_assert(nof_bits_processable % packet_sizes->mcs.N_bps == 0,
                   "nof processable bits is not a multiplex of N_bps");

    dectnrp_assert((PDC_bits_idx < packet_sizes->G && PDC_bits_idx % 8 == 0) ||
                       PDC_bits_idx == packet_sizes->G,
                   "not processed all bits yet, but offset for new bits is not a full byte");

#ifdef PHY_TX_FEC_OF_PDC_CODEBLOCK_FOR_CODEBLOCK_OR_COMPLETE_AT_THE_BEGINNING
    // only channel encode as much data as required
    fec->encode_tb(fec_cfg, *hb_tb, PDC_bits_idx + nof_bits_processable);
#endif

    // map packed bytes to complex symbols
    const uint32_t nof_new_complex_subc = section3::fix::srsran_mod_modulate_bytes_FIX(
        srsran_modem_table_effective,
        hb_tb->get_d(PDC_bits_idx / 8),
        &pdc_cmplx_symbols[PDC_nof_cmplx_subc_residual],
        nof_bits_processable);

    dectnrp_assert(nof_bits_processable / packet_sizes->mcs.N_bps == nof_new_complex_subc,
                   "nof generated complex symbols is not correct");

    PDC_bits_idx += nof_bits_processable;

    // for a non transmit diversity mode, we apply spatial demultiplexing and write directly into
    // the corresponding transmit streams
    if (!transmit_diversity_mode) {
        // go over each subcarrier
        uint32_t cnt = 0;
        for (uint32_t i = 0; i < k_i_one_symbol_size; ++i) {
            // We migh have to write multiple transmit streams per subcarrier.
            // Can this be sped up with an SIMD wrapper?
            for (uint32_t j = 0; j < tm_mode.N_SS; ++j) {
                transmit_streams_stage[j][k_i_one_symbol[i]] = pdc_cmplx_symbols[cnt++];
            }
        }

        if (cnt % tm_mode.N_SS != 0) {
            dectnrp_assert_failure("not a multiple of N_SS many complex symbols written");
        }

        PDC_subc_idx += cnt;
    }
    // for a transmit diversity mode, we have only one spatial stream which is written into the
    // transmit streams
    else {
        // fill flipped version
        for (uint32_t i = 0; i < nof_cmplx_subc; i += 2) {
            pdc_cmplx_symbols_flipped[i] = pdc_cmplx_symbols[i + 1];
            pdc_cmplx_symbols_flipped[i + 1] = pdc_cmplx_symbols[i];
        }

        // change the pattern from   real imag   real imag ... to -real imag   real -imag ...
        // *2 for using float function (fff) instead of complex
        srsran_vec_prod_fff((float*)pdc_cmplx_symbols_flipped,
                            (float*)Y_i->get_pattern(),
                            (float*)pdc_cmplx_symbols_flipped,
                            nof_cmplx_subc * 2);

        // which transmit stream is used for two consecutive complex symbols?
        const common::vec2d<uint32_t>& index_mat_N_TS_x = Y_i->get_index_mat_N_TS_x(tm_mode.N_TS);

        // modulo operator
        const uint32_t modulo = Y_i->get_modulo(tm_mode.N_TS);

        // copy into correct transmit stream at correct subcarrier
        uint32_t i_mod_x, TS_index_A, TS_index_B;
        uint32_t cnt = 0;
        for (uint32_t i : k_i_one_symbol) {
            i_mod_x = (PDC_subc_idx / 2) % modulo;

            TS_index_A = index_mat_N_TS_x[i_mod_x][0];
            TS_index_B = index_mat_N_TS_x[i_mod_x][1];

            transmit_streams_stage[TS_index_A][i] = pdc_cmplx_symbols[cnt];
            transmit_streams_stage[TS_index_B][i] = pdc_cmplx_symbols_flipped[cnt];

            ++cnt;
            ++PDC_subc_idx;
        }
    }

    // How many unused symbols are left now?
    PDC_nof_cmplx_subc_residual =
        PDC_nof_cmplx_subc_residual + nof_new_complex_subc - nof_cmplx_subc;

    dectnrp_assert(PDC_nof_cmplx_subc_residual <= 23,
                   "nof processable bits is not a multiplex of N_bps");

    // copy residual symbols to the front
    srsran_vec_cf_copy(
        pdc_cmplx_symbols, &pdc_cmplx_symbols[nof_cmplx_subc], PDC_nof_cmplx_subc_residual);
}

}  // namespace dectnrp::phy
