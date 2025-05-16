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

#include "dectnrp/phy/rx/rx_synced/rx_synced.hpp"

#include <volk/volk.h>

#include <algorithm>
#include <cmath>

extern "C" {
#include "srsran/phy/modem/demod_soft.h"
#include "srsran/phy/utils/vector.h"
}

#ifdef RX_SYNCED_PARAM_AMPLITUDE_SCALING
extern "C" {
#include "srsran/phy/utils/vector_simd.h"
}
#endif

#include "dectnrp/common/complex.hpp"
#include "dectnrp/common/json/json_export.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_statistics.hpp"
#include "dectnrp/phy/rx/rx_synced/rx_synced_param.hpp"
#include "dectnrp/sections_part3/numerologies.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"
#include "dectnrp/sections_part3/stf.hpp"
#include "dectnrp/sections_part3/transmission_packet_structure.hpp"

namespace dectnrp::phy {

rx_synced_t::rx_synced_t(const radio::buffer_rx_t& buffer_rx_,
                         const worker_pool_config_t& worker_pool_config_,
                         const uint32_t ant_streams_unit_length_samples_)
    : tx_rx_t(worker_pool_config_.maximum_packet_sizes,
              worker_pool_config_.os_min,
              worker_pool_config_.resampler_param),
      rx_pacer_t(
          buffer_rx_.nof_antennas,
          buffer_rx_,
          ant_streams_unit_length_samples_,
          new resampler_t(
              buffer_rx_.nof_antennas,
              // at the receiver L and M are switched, see tx.cpp
              worker_pool_config_.resampler_param.M,
              worker_pool_config_.resampler_param.L,
              resampler_param_t::f_pass_norm[resampler_param_t::user_t::RX_SYNCED][os_min],
              resampler_param_t::f_stop_norm[resampler_param_t::user_t::RX_SYNCED][os_min],
              resampler_param_t::PASSBAND_RIPPLE_DONT_CARE,
              resampler_param_t::f_stop_att_dB[resampler_param_t::user_t::RX_SYNCED][os_min])),
      N_RX(worker_pool_config_.maximum_packet_sizes.tm_mode.N_TX),

      /* We resample in the function resample_until_nto(). The maximum value of resampled samples we
       * require is one full STF.
       */
      localbuffer_resample(get_initialized_localbuffer(
          rx_pacer_t::localbuffer_choice_t::LOCALBUFFER_RESAMPLE,
          section3::stf_t::get_N_samples_stf(worker_pool_config_.maximum_packet_sizes.psdef.u) *
              worker_pool_config_.maximum_packet_sizes.psdef.b * os_min)),

      chestim_mode_lr_default(worker_pool_config_.chestim_mode_lr_default),
      chestim_mode_lr_t_stride(worker_pool_config_.chestim_mode_lr_t_stride_default) {
    // What is the maximum symbol length of a DF symbol without CP in samples?
    // This is also the maximum FFT size we require.
    const uint32_t N_b_DFT_os_max =
        dect_samp_rate_oversampled_max / constants::subcarrier_spacing_min_u_b;

    dectnrp_assert(N_b_DFT_os_max % 4 == 0, "number of subcarrier not a multiple of 4");

    /* What is then the maximum symbol length for an OFDM symbol in samples? The largest conceivable
     * CP is reached for u>=2, with the total OFDM symbol length of (1 + 5/4 = 9/4). The mixing
     * stage must be able to process any STF length.
     */
    const uint32_t N_samples_OFDM_symbol_os_max = N_b_DFT_os_max / 4 * 9;

    /* At the receiver, we make the following assumption about the effective number of antennas a
     * potential transmitter may have:
     *
     * N_TX = N_eff_TX_max, i.e. for a specific number of TX antennas (number defined by the radio
     * device class) we must test for MIMO modes with the same number of effective antennas.
     */
    const uint32_t N_eff_TX_max = worker_pool_config_.maximum_packet_sizes.tm_mode.N_TX;

    for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
        mixer_stage.push_back(srsran_vec_cf_malloc(N_samples_OFDM_symbol_os_max));
    }

    fft_stage = srsran_vec_cf_malloc(N_b_DFT_os_max);
    mrc_stage = srsran_vec_cf_malloc(N_b_DFT_os_max);

    const uint32_t u_max = worker_pool_config_.maximum_packet_sizes.psdef.u;
    const uint32_t b_max = worker_pool_config_.maximum_packet_sizes.psdef.b;
    const uint32_t b_idx_max = section3::phyres::b2b_idx[b_max];
    const uint32_t N_b_OCC_plus_DC_max = section3::phyres::N_b_OCC_plus_DC_lut[b_idx_max];
    const uint32_t processing_stage_len_max = N_eff_TX2processing_stage_len[N_eff_TX_max];

    // we demap upper and lower spectrum separately
    demapping_stage = srsran_vec_u8_malloc(N_b_OCC_plus_DC_max *
                                           worker_pool_config_.maximum_packet_sizes.mcs.N_bps *
                                           PHY_D_RX_DATA_TYPE_SIZE);

    processing_stage = std::make_unique<processing_stage_t<cf_t>>(
        N_b_OCC_plus_DC_max, processing_stage_len_max, N_RX);

    estimator_sto = std::make_unique<estimator_sto_t>(b_max);
    estimator_cfo = std::make_unique<estimator_cfo_t>();
    estimator_snr = std::make_unique<estimator_snr_t>(b_max);
    estimator_mimo = std::make_unique<estimator_mimo_t>(
        N_RX, worker_pool_config_.maximum_packet_sizes.tm_mode.N_TS);
    estimator_aoa = std::make_unique<estimator_aoa_t>(b_max, N_RX);

    const auto numerologies = section3::get_numerologies(u_max, 1);

    // initialize temporary vectors to init channel_luts
    const std::vector<double> V0 = RX_SYNCED_PARAM_NU_MAX_HZ_VEC;
    const std::vector<double> V1 = RX_SYNCED_PARAM_TAU_RMS_SEC_VEC;
    const std::vector<double> V2 = RX_SYNCED_PARAM_SNR_DB_VEC;
    const std::vector<uint32_t> V3 = RX_SYNCED_PARAM_NOF_DRS_INTERP_LR_VEC;
    const std::vector<uint32_t> V4 = RX_SYNCED_PARAM_NOF_DRS_INTERP_L_VEC;

    // init channel_luts
    for (uint32_t i = 0; i < V0.size(); ++i) {
        const channel_statistics_t chst(
            numerologies.delta_u_f, numerologies.T_u_symb, V0[i], V1[i], V2[i], V3[i], V4[i]);

        channel_luts.push_back(std::make_unique<channel_lut_t>(b_max, N_eff_TX_max, chst));
    }

    // initialize for every RX antenna
    for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
        channel_antennas.push_back(std::make_unique<channel_antenna_t>(b_max, N_eff_TX_max));
    }

    hb_rx_plcf = harq::buffer_rx_plcf_t::new_unique_instance();

    plcf_decoder = std::make_unique<section4::plcf_decoder_t>(
        worker_pool_config_.maximum_packet_sizes.psdef.PacketLength,
        worker_pool_config_.maximum_packet_sizes.psdef.mcs_index,
        worker_pool_config_.maximum_packet_sizes.tm_mode.N_SS);

    // prepare for first reception
    reset_for_next_pcc();

    // will be overwritten, but must be preallocated
    ofdm_symbol_now.resize(N_RX);
    std::fill(ofdm_symbol_now.begin(), ofdm_symbol_now.end(), nullptr);

#ifdef PHY_RX_RX_SYNCED_TCP_SCOPE
    tcp_scope = std::make_unique<common::adt::tcp_scope_t<cf_t>>(2200, 1);
#endif
}

rx_synced_t::~rx_synced_t() {
    for (auto& elem : mixer_stage) {
        free(elem);
    }

    free(fft_stage);
    free(mrc_stage);
    free(demapping_stage);
}

pcc_report_t rx_synced_t::demoddecod_rx_pcc(sync_report_t& sync_report_) {
    sync_report = &sync_report_;

#ifdef RX_SYNCED_PARAM_BLOCK_N_EFF_TX_LARGER_1_AT_PCC
    // temporary restriction as not all features are implemented yet
    if (sync_report->N_eff_TX > 1) {
        // indicate that no PLCF was found
        plcf_decoder->set_configuration();

        return pcc_report_t(*plcf_decoder.get());
    }
#endif

    // how many subcarriers does the nominal, oversampled bandwidth contain?
    N_b_DFT_os =
        dect_samp_rate_oversampled_max / (sync_report->u * constants::subcarrier_spacing_min_u_b);

    // set FFT size
    set_ofdm_vec_idx_effective(N_b_DFT_os);

    /* At this point, synchronization is done and the sync_report is complete. It contains, among
     * other things, the values of u and b for the current packet. With the fixed sample rate given,
     * we can thus determine the size of all OFDM symbols in both time- and frequency domain.
     */
    run_symbol_dimensions();

    // determine how many samples we move the fine sync point into the cyclic prefix
    const uint32_t n_samples_into_cp =
        (N_samples_STF_CP_only_os + N_b_DFT_os) *
        RX_SYNCED_PARAM_STO_INTEGER_MOVE_INTO_CP_IN_PERCENTAGE_OF_STF / 100;

    // tell pacer where to start resampling
    reset_localbuffer(rx_pacer_t::localbuffer_choice_t::LOCALBUFFER_RESAMPLE,
                      sync_report->fine_peak_time_64 - int64_t{n_samples_into_cp});

    // the packet will begin at the first sample
    localbuffer_cnt_w = 0;

    /* The sync_report also contains the value N_eff_TX, i.e. how many transmit antennas were used
     * effectively, which is the same as the number of transmit streams transmitted. With this
     * additional information, we can reconfigure the state machines of PCC, DRS and PDC.
     */
    reconfigure_packet_components(sync_report->b, sync_report->N_eff_TX);

    reset_common_counters();

    // set dimensions of current packet in processing stage
    processing_stage->set_configuration(N_b_OCC_plus_DC,
                                        N_eff_TX2processing_stage_len[sync_report->N_eff_TX]);

    localbuffer_cnt_r = 0;

    // setup mixer for CFO correction
    mixer.set_phase(0.0f);

#ifdef RX_SYNCED_PARAM_CFO_CORRECTION
    mixer.set_phase_increment(sync_report->cfo_fractional_rad + sync_report->cfo_integer_rad);
#else
    mixer.set_phase_increment(0.0f);
#endif

    estimator_sto->reset(sync_report->b, sync_report->N_eff_TX);
    estimator_cfo->reset(sync_report->b, sync_report->N_eff_TX);
    estimator_snr->reset(sync_report->b, sync_report->N_eff_TX);
    estimator_mimo->reset(sync_report->b, sync_report->N_eff_TX);
    estimator_aoa->reset(sync_report->b, sync_report->N_eff_TX);

    /* At this point, we know the OFDM symbol sizes but we don't know yet how many OFDM symbols
     * there are in the current packet. To retrieve that information, we have to collect the PCC and
     * decode it. Every PCC cell has to be channel corrected, and chestim_mode_lr=false is mandatory
     * at this stage as we don't even know whether there is a second DRS symbol, and we want
     * minimum latency.
     */
    chestim_mode_lr = false;

    // packets always start with a processing stage index 0
    ps_idx = 0;

    // required in both channel estimation modes
    N_step = drs.get_N_step(sync_report->N_eff_TX);

    /* The STF has an absolute OFDM symbol index 0, the first DF symbol has an absolute symbol index
     * of 1. The first relative symbol index within the processing stage is 0.
     */
    ofdm_symb_ps_idx = 0;

    run_stf(sync_report_);

    /* With the given packet configuration so far, we can also determine the maximum OFDM symbol
     * index to contain PCC cells.
     * Minimum: Looking at Figure 4.5-2 a), when b >> 1 we can fit the entire PCC into the symbol
     *          with index 1. Thus, the minimum value is 1.
     * Maximum: Looking at Figure 4.5-3 e), the maximum value is 4.
     */
    const uint32_t pcc_symbol_idx_max = pcc.get_pcc_symbol_idx_max();

    // collect OFDM symbols until we have all PCC cells put onto the processing stage
    for (ofdm_symb_idx = 1; ofdm_symb_idx <= pcc_symbol_idx_max; ++ofdm_symb_idx) {
        // collect one OFDM symbol and ...
        run_mix_resample(N_b_CP_os);

        // ... FFT directly onto processing stage
        run_cp_fft_scale(N_b_CP_os);

        // is true for ofdm_symb_idx==1, can be true for ofdm_symb_idx==2 if N_eff_TX=8
        if (drs.is_symbol_index(ofdm_symb_idx)) {
            run_drs_chestim_zf();
            run_drs_ch_interpolation();
        }

        // collect PCC from cells and demap
        if (pcc.is_symbol_index(ofdm_symb_idx)) {
            run_pcc_collection_and_demapping();
        }

        ++ofdm_symb_ps_idx;
    }

    dectnrp_assert(PCC_idx == constants::pcc_cells, "Incorrect number of PCC subcarriers read");

    run_pcc_decoding_and_candidate_search();

#ifdef RX_SYNCED_PARAM_BLOCK_N_SS_TX_LARGER_1_AT_PCC
    // temporary restriction as not all features are implemented yet
    const auto* type1_N_SS_check = plcf_decoder->get_plcf_base(1);
    const auto* type2_N_SS_check = plcf_decoder->get_plcf_base(2);
    if ((type1_N_SS_check != nullptr && type1_N_SS_check->get_N_SS() > 1) ||
        (type2_N_SS_check != nullptr && type2_N_SS_check->get_N_SS() > 1)) {
        // indicate that no PLCF was found
        plcf_decoder->set_configuration();

        return pcc_report_t(*plcf_decoder.get());
    }
#endif

    // compile results for lower MAC
    return pcc_report_t(*plcf_decoder.get(), estimator_snr->get_current_snr_dB_estimation());
}

pdc_report_t rx_synced_t::demoddecod_rx_pdc(const maclow_phy_t& maclow_phy_) {
    maclow_phy = &maclow_phy_;
    packet_sizes = &maclow_phy_.hp_rx->get_packet_sizes();
    hb_tb = maclow_phy_.hp_rx->get_hb_tb();

#ifdef RX_SYNCED_PARAM_BLOCK_N_EFF_TX_LARGER_1_AT_PDC
    // temporary restriction as not all features are implemented yet
    if (packet_sizes->tm_mode.N_eff_TX > 1) {
        return pdc_report_t(mac_pdu_decoder);
    }
#endif

    /* At this point, we have decoded the PCC and called the lower MAC of the tpoint. The tpoint
     * made the decision to decode the PDC as well, calculated the packet size and passed on a HARQ
     * process.
     */

    // configure all fields of FEC
    fec_cfg.PLCF_type = maclow_phy->hp_rx->get_PLCF_type();
    // fec_cfg.closed_loop      // irrelevant for PDC
    // fec_cfg.beamforming      // irrelevant for PDC
    fec_cfg.N_TB_bits = packet_sizes->N_TB_bits;
    fec_cfg.N_bps = packet_sizes->mcs.N_bps;
    fec_cfg.rv = maclow_phy->hp_rx->get_rv();
    fec_cfg.G = packet_sizes->G;
    fec_cfg.network_id = maclow_phy->hp_rx->get_network_id();
    fec_cfg.Z = packet_sizes->psdef.Z;

    fec->segmentate_and_pick_scrambling_sequence(fec_cfg);

    dectnrp_assert(packet_sizes->psdef.mcs_index <= 9, "Unable to set modem table for MCS index");

    // set symbol demapper
    switch (packet_sizes->psdef.mcs_index) {
        // BPSK
        case 0:
            srsran_mod = SRSRAN_MOD_BPSK;
            break;
        // QPSK
        case 1:
        case 2:
            srsran_mod = SRSRAN_MOD_QPSK;
            break;
        // 16 QAM
        case 3:
        case 4:
            srsran_mod = SRSRAN_MOD_16QAM;
            break;
        // 64 QAM
        case 5:
        case 6:
        case 7:
            srsran_mod = SRSRAN_MOD_64QAM;
            break;
        // 256 QAM
        case 8:
        case 9:
        default:
            srsran_mod = SRSRAN_MOD_256QAM;
            break;
    }

    dectnrp_assert(ofdm_symb_idx <= packet_sizes->N_DF_symb,
                   "Absolute OFDM symbol index too large.");

    // reset the MAC PDU decoder
    mac_pdu_decoder.set_configuration(
        maclow_phy_.hp_rx->get_hb_tb()->get_a(), packet_sizes->N_TB_byte, packet_sizes->psdef.u);

    // process PDC based on mode
    if (chestim_mode_lr_default) {
        run_pdc_ps_in_chestim_mode_lr_t();
    }
    run_pdc_ps_in_chestim_mode_lr_f();

    // run some checks after packet processing
    dectnrp_assert(PCC_idx == constants::pcc_cells, "incorrect number of PCC subcarriers written");
    dectnrp_assert(DRS_idx == packet_sizes->N_DRS_subc,
                   "incorrect number of DRS subcarriers written");
    dectnrp_assert(fec->get_wp() - 24 == packet_sizes->N_TB_bits,
                   "incorrect position of FEC write pointer bits written");
    dectnrp_assert(PDC_bits_idx == packet_sizes->G, "incorrect number of PDC bits written");
    dectnrp_assert(PDC_subc_idx == packet_sizes->N_PDC_subc * packet_sizes->tm_mode.N_SS,
                   "incorrect number of PDC subcarriers written across all transmit streams");
    dectnrp_assert(mac_pdu_decoder.has_reached_valid_final_state(),
                   "mac_pdu_decoder invalid final state");

    // CRC must be correct to call MAC, however, MAC PDU still can be ill-formed
    if (!fec->get_decode_tb_status_latest()) {
        return pdc_report_t(mac_pdu_decoder);
    }

#ifdef RX_SYNCED_PARAM_MIMO_BASED_ON_STF_AND_DRS_AT_PACKET_END
    /* At this point, the latest transmit stream indices are not relevant, as at the end of a packet
     * it is always guaranteed that all transmit streams are present in the channel estimate.
     */
    const process_drs_meta_t process_drs_meta(
        sync_report->N_eff_TX, TS_idx_first, TS_idx_last, ofdm_symb_idx);

    dectnrp_assert(TS_idx_last == sync_report->N_eff_TX - 1,
                   "last transmit stream incorrect {} {}",
                   TS_idx_last,
                   sync_report->N_eff_TX);

    // use derotated and packed DRS symbols
    estimator_mimo->process_drs(channel_antennas, process_drs_meta);
#endif

    return pdc_report_t(mac_pdu_decoder,
                        estimator_snr->get_current_snr_dB_estimation(),
                        estimator_mimo->get_mimo_report());
}

void rx_synced_t::reset_for_next_pcc() {
    hb_rx_plcf->reset_a_cnt_and_softbuffer();
    channel_lut_effective = nullptr;
}

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
nlohmann::ordered_json rx_synced_t::get_json() const {
    nlohmann::ordered_json json;

    json["snr"] = estimator_snr->get_current_snr_dB_estimation();
    json["mcs"] = packet_sizes->psdef.mcs_index;

    // we may export less information than available
    const uint32_t N_RX_export = std::min(N_RX, limits::dectnrp_max_nof_antennas);
    const uint32_t N_eff_TX_export =
        std::min(sync_report->N_eff_TX, limits::dectnrp_max_nof_antennas);

    /* The channel is interpolated across all N_b_OCC_plus_DC-many subcarriers. We always start with
     * the first subcarrier, but then we can limit the number of complex channel estimates by using
     * a stride.
     */
    const size_t stride = 1;

    json["N_b_DFT"] = N_b_DFT;
    json["N_b_OCC_plus_DC"] = N_b_OCC_plus_DC;
    json["stride"] = stride;

    // RX antennas
    for (size_t ant_idx = 0; ant_idx < N_RX_export; ++ant_idx) {
        // TX transmit streams
        for (size_t ts_idx = 0; ts_idx < N_eff_TX_export; ++ts_idx) {
            // define the key
            const std::string ch_key =
                "ch_" + std::to_string(ant_idx) + "_" + std::to_string(ts_idx);

            // last known channel measurement
            json[ch_key] = common::json_export_t::convert_32fc_re_im<true, stride>(
                channel_antennas.at(ant_idx)->chestim.at(ts_idx), N_b_OCC_plus_DC);
        }
    }

    return json;
}
#endif

void rx_synced_t::run_symbol_dimensions() {
    const uint32_t b_idx = section3::phyres::b2b_idx[sync_report->b];

    N_b_DFT = section3::phyres::N_b_DFT_lut[b_idx];
    N_b_OCC = section3::phyres::N_b_OCC_lut[b_idx];
    N_b_OCC_plus_DC = N_b_OCC + 1;

    set_N_subc_offset_lower_half_os(
        N_b_DFT_os, N_b_DFT, section3::phyres::guards_bottom_lut[b_idx]);

    // cyclic prefix lengths without oversampling
    N_samples_STF_CP_only_os = section3::transmission_packet_structure::get_N_samples_STF_CP_only(
        sync_report->u, sync_report->b);
    N_b_CP_os =
        section3::transmission_packet_structure::get_N_samples_OFDM_symbol_CP_only(sync_report->b);

    // cyclic prefix lengths with oversampling
    N_samples_STF_CP_only_os *= N_b_DFT_os / N_b_DFT;
    N_b_CP_os *= N_b_DFT_os / N_b_DFT;  // same as N_b_DFT_os / 8
}

void rx_synced_t::run_stf(sync_report_t& sync_report_) {
    /* Resample until the STF is available. Right after resampling, the STF is mixed with the
     * fractional+integer CFO provided by the synchronization. That CFO is already precise.
     */
    run_mix_resample(N_samples_STF_CP_only_os);

    run_stf_rms_estimation(sync_report_);

    const uint32_t N_samples_STF_os = N_samples_STF_CP_only_os + N_b_DFT_os;
    const uint32_t nof_pattern = section3::stf_t::get_N_stf_pattern(sync_report_.u);
    const uint32_t nof_samples_pattern = N_samples_STF_os / nof_pattern;

    // revert the cover sequence by reapplying it
    section3::stf_t::apply_cover_sequence(
        mixer_stage, mixer_stage, N_RX, sync_report_.u, nof_samples_pattern);

    // Remove STF amplitude boost applied at TX. This is optional as most algorithms do not depend
    // on the actual magnitude of the STF.
    // \todo

#ifdef RX_SYNCED_PARAM_CFO_FRACTIONAL_ADJUST
    /* The STF in time domain is now available on mixer_stage. It is resampled to a DECT NR+
     * sample rate, starting from the fine synchronization time. The cover sequence has been
     * reverted. The STF is now used to estimate the fractional CFO once again. Typically, the
     * amount of fractional CFO still left is very small. It is applied by changing the current CFO
     * value of the mixer.
     */

    // we compare neighbouring pairs of patterns and estimate the average phase rotation
    lv_32fc_t sum = lv_cmake(0.0f, 0.0f);
    lv_32fc_t sum_partial;

    // go over each RX antenna stream ...
    for (uint32_t i = 0; i < N_RX; ++i) {
        // ... and every pair of STF patterns
        for (uint32_t j = 0; j < nof_pattern - 1; ++j) {
            volk_32fc_x2_conjugate_dot_prod_32fc(
                &sum_partial,
                (const lv_32fc_t*)&mixer_stage[i][j * nof_samples_pattern],
                (const lv_32fc_t*)&mixer_stage[i][(j + 1) * nof_samples_pattern],
                nof_samples_pattern);

            // sum across all patterns
            sum += sum_partial;
        }
    }

    const float cfo_fractional_left_rad = std::arg(sum) / static_cast<float>(nof_samples_pattern);

    sync_report_.cfo_fractional_rad += cfo_fractional_left_rad;

#ifdef RX_SYNCED_PARAM_CFO_CORRECTION
    mixer.adjust_phase_increment(cfo_fractional_left_rad);
#endif

#endif

    // FFT directly onto processing stage
    run_cp_fft_scale(N_samples_STF_CP_only_os);

    // estimate channel of STF based on ofdm_symbol_now
    run_stf_chestim_zf();

#if defined(RX_SYNCED_PARAM_STO_FRACTIONAL_BASED_ON_STF) || \
    defined(RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_STF) ||   \
    defined(RX_SYNCED_PARAM_SNR_BASED_ON_STF) ||            \
    defined(RX_SYNCED_PARAM_MIMO_BASED_ON_STF_AND_DRS_AT_PACKET_END)
    const process_stf_meta_t process_stf_meta(sync_report_.fine_peak_time_64);
#endif

#ifdef RX_SYNCED_PARAM_STO_FRACTIONAL_BASED_ON_STF
    // run estimator_sto estimation based on STF
    estimator_sto->process_stf(channel_antennas, process_stf_meta);

    // immediately correct phase rotation of the current OFDM symbol for upcoming CFO estimation
    estimator_sto->apply_full_phase_rotation(ofdm_symbol_now);

    // overwrite fractional STO in sync_report
    sync_report_.sto_fractional = estimator_sto->get_fractional_sto_in_samples(N_b_DFT_os);

    dectnrp_assert(std::abs(sync_report_.sto_fractional) < static_cast<float>(N_b_DFT / 4),
                   "fractional STO very large");

    // overwrite exact fine peak time in sync_report
    sync_report_.fine_peak_time_corrected_by_sto_fractional_64 =
        sync_report_.fine_peak_time_64 +
        static_cast<int64_t>(std::round(sync_report_.sto_fractional));

#if defined(RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_STF) || defined(RX_SYNCED_PARAM_SNR_BASED_ON_STF)
    /* At this point, we have correct the phase rotation of ofdm_symbol_now. If the STF is required
     * for either estimator_cfo or estimator_snr, we must reestimate the channel of the STF to
     * achieve proper results.
     */
    run_stf_chestim_zf();
#endif

#endif

#ifdef RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_STF
    estimator_cfo->process_stf(channel_antennas, process_stf_meta);

    /* At this point, estimator_cfo was provided with the STF only. Two measure the common phase
     * error, it requires at least two distinct OFDM symbols to measure the rotation. Thus, at this
     * point we cannot adjust the mixer yet. This is possible once at least one DRS symbol has been
     * provided.
     */
#endif

#ifdef RX_SYNCED_PARAM_SNR_BASED_ON_STF
    // the current SNR value is required for LUT choice and for reporting to MAC
    estimator_snr->process_stf(channel_antennas, process_stf_meta);
#endif

#ifdef RX_SYNCED_PARAM_MIMO_BASED_ON_STF_AND_DRS_AT_PACKET_END
    estimator_mimo->process_stf(channel_antennas, process_stf_meta);
#endif
}

void rx_synced_t::run_stf_rms_estimation(sync_report_t& sync_report_) {
    dectnrp_assert(sync_report_.rms_array.has_any_larger(0.0f),
                   "not a single RMS value provided by sync");

    common::ant_t rms_array(N_RX);

#ifdef RX_SYNCED_PARAM_RMS_FILL_COMPLETELY_OR_KEEP_WHAT_SYNCHRONIZATION_PROVIDED
    // full lenght of the STF with oversampling
    const uint32_t N_samples_STF_os = N_samples_STF_CP_only_os + N_b_DFT_os;

    // where and how much do we multiply?
    const uint32_t length =
        N_samples_STF_os * RX_SYNCED_PARAM_RMS_PERCENTAGE_OF_STF_USED_FOR_RMS_ESTIMATION / 100;
    const uint32_t offset = N_samples_STF_os - length;

    // estimate the RMS at each antenna
    for (uint32_t i = 0; i < N_RX; ++i) {
#ifdef RX_SYNCED_PARAM_RMS_KEEP_VALUES_PROVIDED_BY_SYNC
        if (i < sync_report_.rms_array.get_nof_antennas() && 0.0f < sync_report_.rms_array.at(i)) {
            rms_array.at(i) = sync_report_.rms_array.at(i);
            continue;
        }
#endif
        lv_32fc_t rms = lv_cmake(0.0f, 0.0f);

        volk_32fc_x2_conjugate_dot_prod_32fc(&rms,
                                             (const lv_32fc_t*)&mixer_stage[i][offset],
                                             (const lv_32fc_t*)&mixer_stage[i][offset],
                                             length);

        rms_array.at(i) = std::sqrt(std::abs(rms) / static_cast<float>(length));
    }
#else
    // simply copy values from synchronization to new array with correct number of antennas
    for (std::size_t i = 0; i < sync_report_.rms_array.get_nof_antennas(); ++i) {
        rms_array.at(i) = sync_report_.rms_array.at(i);
    }
#endif

    sync_report_.rms_array = rms_array;
}

void rx_synced_t::run_stf_chestim_zf() {
    // load values of STF
    const auto& y_STF = stf.get_stf(sync_report->b, sync_report->N_eff_TX);

    // number of occupied STF cells
    const std::size_t N_STF_cells_b = sync_report->b * constants::N_STF_cells_b_1;

    // estimate channel across all antennas
    for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
        // pointer to current OFDM symbol of current RX antenna
        const cf_t* ofdm_symbol_now_ant = ofdm_symbol_now[ant_idx];

        // container for consecutive channel estimations at STF cells, by convention transmit stream
        // 0 is used
        cf_t* chestim_drs_zf_0 = channel_antennas[ant_idx]->chestim_drs_zf[0];

        // absolute subcarrier offset starting from the first occupied STF cell
        std::size_t r_idx = 0;
        std::size_t w_idx = 0;

        // lower part of spectrum
        for (std::size_t i = 0; i < N_STF_cells_b / 2 - 1; ++i) {
            // pseudo code: chestim = (received cell value) / (transmitted cell value)
            chestim_drs_zf_0[w_idx] = ofdm_symbol_now_ant[r_idx] / y_STF.at(r_idx);

            r_idx += constants::N_STF_cells_separation;
            ++w_idx;
        }

        // last occupied STF cell before DC
        chestim_drs_zf_0[w_idx] = ofdm_symbol_now_ant[r_idx] / y_STF.at(r_idx);

        // center STF cell
        r_idx += constants::N_STF_cells_separation_center;
        ++w_idx;

        // upper part of spectrum
        for (std::size_t i = 0; i < N_STF_cells_b / 2; ++i) {
            // pseudo code: chestim = (received cell value) / (transmitted cell value)
            chestim_drs_zf_0[w_idx] = ofdm_symbol_now_ant[r_idx] / y_STF.at(r_idx);

            // if will be always for at this point
            r_idx += constants::N_STF_cells_separation;
            ++w_idx;
        }
    }
}

void rx_synced_t::run_mix_resample(const uint32_t N_samples_in_CP_os) {
    // what is the length of an OFDM symbol for the current configuration?
    const uint32_t N_samples_OFDM_symbol_os = N_samples_in_CP_os + N_b_DFT_os;

    // how many samples are left from the last resampling?
    const uint32_t residual_have = localbuffer_cnt_w - localbuffer_cnt_r;

    // do we have the entire symbol collected?
    if (residual_have >= N_samples_OFDM_symbol_os) {
        // copy entire symbol onto mixing stage
        mixer.mix_phase_continuous_offset(
            localbuffer_resample, localbuffer_cnt_r, mixer_stage, 0, N_samples_OFDM_symbol_os);

        localbuffer_cnt_r += N_samples_OFDM_symbol_os;
    } else {
        // copy what we have onto mixing stage
        mixer.mix_phase_continuous_offset(
            localbuffer_resample, localbuffer_cnt_r, mixer_stage, 0, residual_have);

        // how many more samples do we need?
        const uint32_t residual_need = N_samples_OFDM_symbol_os - residual_have;

        // resample required amount of samples starting from the front of the local buffer
        rewind_localbuffer_resample_cnt_w();
        localbuffer_cnt_w = resample_until_nto(residual_need);

        // copy rest onto mixing stage
        mixer.mix_phase_continuous_offset(
            localbuffer_resample, 0, mixer_stage, residual_have, residual_need);

        localbuffer_cnt_r = residual_need;
    }

    // update pointers to current OFDM symbol
    processing_stage->get_stage_prealloc(ofdm_symb_ps_idx, ofdm_symbol_now);
}

void rx_synced_t::run_cp_fft_scale(const uint32_t N_samples_in_CP_os) {
    for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
        // transform onto FFT stage, implicitly skip CP
        dft::single_symbol_rx_ofdm_zero_copy(
            ofdm_vec[ofdm_vec_idx_effective], mixer_stage[ant_idx], fft_stage, N_samples_in_CP_os);

        // copy upper and lower half of spectrum from FFT stage into processing unit
        srsran_vec_cf_copy(&ofdm_symbol_now[ant_idx][N_b_OCC / 2], fft_stage, N_b_OCC / 2 + 1);
        srsran_vec_cf_copy(
            ofdm_symbol_now[ant_idx], &fft_stage[N_subc_offset_lower_half_os], N_b_OCC / 2);

#ifdef RX_SYNCED_PARAM_AMPLITUDE_SCALING
        // not required when using channel estimation with floats, but useful for debugging
        const float scale_factor = std::sqrt(float(N_b_OCC)) / float(N_b_DFT_os);
        srsran_vec_sc_prod_cfc_simd(
            ofdm_symbol_now[ant_idx], scale_factor, ofdm_symbol_now[ant_idx], N_b_OCC_plus_DC);
#endif
    }

#if defined(RX_SYNCED_PARAM_STO_FRACTIONAL_BASED_ON_STF) || \
    defined(RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS)
    estimator_sto->apply_full_phase_rotation(ofdm_symbol_now);
#endif
}

void rx_synced_t::run_drs_chestim_zf() {
    dectnrp_assert(!(!chestim_mode_lr && sync_report->N_eff_TX <= 4 && ofdm_symb_ps_idx != 0),
                   "index cannot contain DRS cells");
    dectnrp_assert(!(!chestim_mode_lr && sync_report->N_eff_TX > 4 && ofdm_symb_ps_idx > 1),
                   "index cannot contain DRS cells");

    // load DRS information for all TS, k_i are the indices, y_DRS_i are the complex values
    const common::vec2d<uint32_t>& k_i = drs.get_k_i();
    const common::vec2d<cf_t>& y_DRS_i = drs.get_y_DRS_i();

    // number of DRS cells
    const uint32_t k_i_size = k_i[0].size();

    // load indices of transmit streams of this symbol
    TS_idx_first = drs.get_ts_idx_first();
    TS_idx_last = drs.get_ts_idx_last_and_advance();

    // count for final assert
    DRS_idx += k_i_size * (TS_idx_last - TS_idx_first + 1);

    // offset for individial transmit streams for interlacing
    const auto& w_offsets = channel_antenna_t::get_write_offsets(ps_idx, ofdm_symb_ps_idx);

    // Step 3) Go over each RX antenna's OFDM symbol in frequency domain ...
    for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
        // pointer to current OFDM symbol of current RX antenna
        const cf_t* ofdm_symbol_now_ant = ofdm_symbol_now[ant_idx];

        // Step 3.1) Go over each transmit stream ...
        for (uint32_t ts_idx = TS_idx_first; ts_idx <= TS_idx_last; ++ts_idx) {
            // DRS cell indices and values of current TS
            const std::vector<uint32_t>& k_i_this_ts = k_i[ts_idx % 4];
            const std::vector<cf_t>& y_DRS_i_this_ts = y_DRS_i[ts_idx % 4];

            // container for consecutive channel estimations at DRS cells
            cf_t* chestim_drs_zf_ts = channel_antennas[ant_idx]->chestim_drs_zf[ts_idx];

            // container for consecutive channel estimations at DRS cells
            cf_t* chestim_drs_zf_interlaced_ts =
                channel_antennas[ant_idx]->chestim_drs_zf_interlaced[ts_idx];

            // start writing at correct offset
            uint32_t w_idx = 0;
            uint32_t w_idx_interlaced = w_offsets[ts_idx];

            // ... and estimate the channel at each DRS cell by zero-forcing
            for (uint32_t i = 0; i < k_i_size; ++i) {
                // pseudo code: chestim = (received cell value) / (transmitted cell value)
                const cf_t A = ofdm_symbol_now_ant[k_i_this_ts[i]] / y_DRS_i_this_ts[i];

                chestim_drs_zf_ts[w_idx] = A;
                chestim_drs_zf_interlaced_ts[w_idx_interlaced] = A;

                ++w_idx;
                w_idx_interlaced += 2U;
            }
        }
    }

#if defined(RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS) || \
    defined(RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_DRS) || \
    defined(RX_SYNCED_PARAM_SNR_BASED_ON_DRS)
    const process_drs_meta_t process_drs_meta(
        sync_report->N_eff_TX, TS_idx_first, TS_idx_last, ofdm_symb_idx);
#endif

#ifdef RX_SYNCED_PARAM_STO_RESIDUAL_BASED_ON_DRS
    estimator_sto->process_drs(channel_antennas, process_drs_meta);
#endif

#ifdef RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_DRS
    estimator_cfo->process_drs(channel_antennas, process_drs_meta);

    // we require the phase shift in time domain from sample to sample (s2s)
    const float A = estimator_cfo->get_residual_CFO_s2s_rad(N_b_CP_os + N_b_DFT_os);

    // mixer already is set to correct integer + fractional CFO, here we can make small adjustments
    // to add residual CFO correction
    mixer.adjust_phase_increment(-A);
#endif

#ifdef RX_SYNCED_PARAM_SNR_BASED_ON_DRS
    // use derotated and packed DRS symbols for SNR estimation
    estimator_snr->process_drs(channel_antennas, process_drs_meta);
#endif

    // the SNR estimation has improved, find optimal Wiener filter
    run_drs_channel_lut_pick();
}

void rx_synced_t::run_drs_channel_lut_pick() {
#ifndef RX_SYNCED_PARAM_CHANNEL_LUT_LOOKUP_AFTER_EVERY_DRS_SYMBOL_OR_ONCE
    //  we must at least one at the beginning choose a LUT
    if (channel_lut_effective == nullptr) {
#endif
        // load current SNR estimation
        const float snr_now = estimator_snr->get_current_snr_dB_estimation();

        // find channel_luts element with best closest SNR
        uint32_t idx = 0;
        float snr_diff = std::fabs(snr_now - channel_luts[idx]->channel_statistics.snr_db);
        for (uint32_t i = 1; i < channel_luts.size(); ++i) {
            const float s = std::fabs(snr_now - channel_luts[i]->channel_statistics.snr_db);
            if (s <= snr_diff) {
                snr_diff = s;
                idx = i;
            }
        }

        channel_lut_effective = channel_luts[idx].get();

#ifndef RX_SYNCED_PARAM_CHANNEL_LUT_LOOKUP_AFTER_EVERY_DRS_SYMBOL_OR_ONCE
    }
#endif

    // configure parameters of channel estimation LUTs that are constant for the entire packet
    channel_lut_effective->set_configuration_packet(section3::phyres::b2b_idx[sync_report->b],
                                                    sync_report->N_eff_TX);
}

void rx_synced_t::run_drs_ch_interpolation() {
    // tell channel_lut which LUT to use
    channel_lut_effective->set_configuration_ps(chestim_mode_lr, ps_idx);

    // load correct LUT pointers for interpolation
    const auto& idx_pilot = channel_lut_effective->get_idx_pilot_symb(ofdm_symb_ps_idx);
    const auto& idx_weights = channel_lut_effective->get_idx_weights_symb(ofdm_symb_ps_idx);
    const auto weight_vecs = channel_lut_effective->get_weight_vecs();

    // number of consecutive pilots for interpolation
    const uint32_t nof_drs_subc_interp = channel_lut_effective->get_nof_drs_subc_interp();

    // if chestim_mode_lr=true, all transmit streams are available, otherwise only those of the last
    // OFDM symbol with DRS cells
    const uint32_t TS_idx_first_local = chestim_mode_lr ? 0 : TS_idx_first;

    // Step 3) Go over each RX antenna's OFDM symbol in frequency domain ...
    for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
        /* Depending on chestim_mode_lr, either the interlaced or non-interlaced pilots are used for
         * channel interpolation, smoothing and extrapolation. The LUT channel_lut_effective uses
         * the same configuration due to the call of set_configuration_ps().
         */
        const std::vector<cf_t*>& chestim_drs_zf_effective =
            chestim_mode_lr ? channel_antennas[ant_idx]->chestim_drs_zf_interlaced
                            : channel_antennas[ant_idx]->chestim_drs_zf;

        // Step 3.1) Go over each transmit stream ...
        for (uint32_t ts_idx = TS_idx_first_local; ts_idx <= TS_idx_last; ++ts_idx) {
            // LUT vector of current transmit stream
            const RX_SYNCED_PARAM_LUT_IDX_TYPE* idx_pilot_this_ts = idx_pilot[ts_idx % 4];
            const RX_SYNCED_PARAM_LUT_IDX_TYPE* idx_weights_this_ts = idx_weights[ts_idx % 4];

            // container for consecutive channel estimations at DRS cells
            const cf_t* chestim_drs_zf_ts = chestim_drs_zf_effective[ts_idx];

            // target
            cf_t* chestim_ts = channel_antennas[ant_idx]->chestim[ts_idx];

            // ... and estimate the channel at each subcarrier
            for (uint32_t subc_idx = 0; subc_idx < N_b_OCC_plus_DC; ++subc_idx) {
#if RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL
                volk_32fc_32f_dot_prod_32fc_u(
                    (lv_32fc_t*)&chestim_ts[subc_idx],
                    (lv_32fc_t*)&chestim_drs_zf_ts[idx_pilot_this_ts[subc_idx]],
                    &weight_vecs[idx_weights_this_ts[subc_idx] * nof_drs_subc_interp],
                    nof_drs_subc_interp);
#elif RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP
                volk_32fc_x2_dot_prod_32fc_u(
                    (lv_32fc_t*)&chestim_ts[subc_idx],
                    (lv_32fc_t*)&chestim_drs_zf_ts[idx_pilot_this_ts[subc_idx]],
                    (lv_32fc_t*)&weight_vecs[idx_weights_this_ts[subc_idx] * nof_drs_subc_interp],
                    nof_drs_subc_interp);
#endif
            }
        }
    }
}

void rx_synced_t::run_pcc_collection_and_demapping() {
    // load indices of PCC cells in this OFDM symbol
    const std::vector<uint32_t>& k_i_one_symbol = pcc.get_k_i_one_symbol();

    /* With and without transmit diversity, we have to collect the signals across multiple antennas
     * in a numerator and a denominator as in https://en.wikipedia.org/wiki/Maximal-ratio_combining.
     * For this, we reuse mixer_stage and fft_stage as they are no longer required for the current
     * OFDM symbol.
     */
    srsran_vec_cf_zero(mixer_stage[0], k_i_one_symbol.size());
    srsran_vec_cf_zero(fft_stage, k_i_one_symbol.size());

    // only one transmit stream?
    if (sync_report->N_eff_TX == 1) {
        // https://en.wikipedia.org/wiki/Maximal-ratio_combining

        // go over each RX antenna
        for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
            // y_mrc is the received OFDM symbol at this antenna
            const cf_t* y_mrc = ofdm_symbol_now[ant_idx];

            // h_mrc is the estimated channel at this antenna in transmit stream 0
            const cf_t* h_mrc = channel_antennas[ant_idx]->chestim[0];

            // collect all PCC cells
            uint32_t cnt = 0;
            for (uint32_t k_i : k_i_one_symbol) {
                // MRC: conj(h)
                const cf_t h_conj_mrc = conjj(h_mrc[k_i]);

                // MRC numerator: y*conj(h)
                mixer_stage[0][cnt] += y_mrc[k_i] * h_conj_mrc;

                // MRC denominator: |h|^2 = h*conj(h)
                fft_stage[cnt++] += h_mrc[k_i] * h_conj_mrc;
            }
        }

        // MRC numerator/denomimator, technically not required as PCC always uses QPSK
        volk_32fc_x2_divide_32fc((lv_32fc_t*)mixer_stage[0],
                                 (const lv_32fc_t*)mixer_stage[0],
                                 (const lv_32fc_t*)fft_stage,
                                 k_i_one_symbol.size());
    } else {
        run_pxx_mode_transmit_diversity(k_i_one_symbol, PCC_idx);
    }

    // demap: all pointers must be memory-aligned!
    srsran_demod_soft_demodulate_s(
        SRSRAN_MOD_QPSK, mixer_stage[0], (short*)demapping_stage, k_i_one_symbol.size());

    // copy from demapping stage to d-bits of HARQ buffer
    memcpy(hb_rx_plcf->get_d(PCC_idx * 2 * PHY_D_RX_DATA_TYPE_SIZE),
           demapping_stage,
           k_i_one_symbol.size() * 2 * PHY_D_RX_DATA_TYPE_SIZE);

    // update by amount of newly written PCC cells
    PCC_idx += k_i_one_symbol.size();
}

void rx_synced_t::run_pcc_decoding_and_candidate_search() {
    // assume the worst case, i.e. that no PCC will be found
    plcf_decoder->set_configuration();

    // decode type=1 and check CRC
    if (fec->decode_plcf_test(plcf_decoder->get_fec_cfg(1), *hb_rx_plcf.get(), 1)) {
        // CRC correct, so interpret bits
        plcf_decoder->decode_and_rdc_check(1, hb_rx_plcf->get_a());
    }

    // decode type=2 and check CRC
    if (fec->decode_plcf_test(plcf_decoder->get_fec_cfg(2), *hb_rx_plcf.get(), 2)) {
        // CRC correct, so interpret bits
        plcf_decoder->decode_and_rdc_check(2, hb_rx_plcf->get_a());
    }
}

void rx_synced_t::run_pdc_ps_in_chestim_mode_lr_t() {
    // How many OFDM symbols does a processing stage contain in this mode? To accomodate two DRS
    // symbols per transmit stream, the length must be either 6, 11 or 12.
    const uint32_t ps_t_length = N_eff_TX2processing_stage_len[sync_report->N_eff_TX];

    // how many full processing stages are there in the current packet?
    const uint32_t nof_full_ps = (packet_sizes->N_DF_symb - (ps_t_length - N_step)) / N_step;

    // no full-length processing stages? then leave
    if (nof_full_ps == 0) {
        return;
    }

    // switch mode
    chestim_mode_lr = true;

    // process until all processing stages are done
    while (ps_idx < nof_full_ps) {
        // what are the first and final absolute OFDM symbol indices within this processing stage?
        const uint32_t first_idx = 1 + ps_idx * N_step;
        const uint32_t final_idx = ps_t_length + ps_idx * N_step;

        /* When we are in the first processing stage with index 0, we already have processed some
         * OFDM symbols during PCC extraction and written them to the processing stage. Thus, we
         * continue at the last known absolute and relative OFDM symbol indices.
         *
         * When in a later processing stage, we always start at the absolute OFDM symbol index
         * first_idx+1, as the OFDM symbol with absolute index first_idx was already processed as
         * part of the last processing stage. The corresponding relative OFDM symbol index is 1,
         * which we set here.
         */
        if (ps_idx > 0) {
            ofdm_symb_ps_idx = 1;
        }

        // continue at last absolute symbol index
        for (; ofdm_symb_idx <= final_idx; ++ofdm_symb_idx) {
            run_mix_resample(N_b_CP_os);

            run_cp_fft_scale(N_b_CP_os);

            if (drs.is_symbol_index(ofdm_symb_idx)) {
                run_drs_chestim_zf();
            }

            ++ofdm_symb_ps_idx;
        }

#if defined(RX_SYNCED_PARAM_CFO_RESIDUAL_BASED_ON_DRS)
        run_pdc_ps_in_chestim_mode_lr_t_residual_cfo_post_correction();
#endif

        /* We now have collected the channel estimates at the DRS cells to the left and right of the
         * processing stage. Now we have to go over the entire processing stage one more time and
         * collect the PDC. When going over the processing stage, we don't use or change the value
         * of the absolute index ofdm_symb_idx, but only the relative index ofdm_symb_ps_idx.
         *
         * When in processing stage with index 0, we start at the relative index 0. When in a later
         * processing stage, we start at the relative symbol index 1, as the OFDM symbol with
         * relative index 0 was processed as part of the last processing stage.
         */
        const uint32_t relative_start = (ps_idx == 0) ? 0 : 1;

        // go over entire processing stage one more time to collect the PDC, use a relative index
        for (ofdm_symb_ps_idx = relative_start; ofdm_symb_ps_idx <= N_step; ++ofdm_symb_ps_idx) {
            // update pointers to current OFDM symbol
            processing_stage->get_stage_prealloc(ofdm_symb_ps_idx, ofdm_symbol_now);

            // update channel estimation
            if ((ofdm_symb_ps_idx - relative_start) % chestim_mode_lr_t_stride == 0) {
                run_drs_ch_interpolation();
            }

            if (pdc.is_symbol_index(first_idx + ofdm_symb_ps_idx)) {
                run_pdc();
            }
        }

        // update to values of next processing stage
        ++ps_idx;
        ofdm_symb_ps_idx = 0;
    }
}

void rx_synced_t::run_pdc_ps_in_chestim_mode_lr_f() {
    // set correct mode
    chestim_mode_lr = false;

    // how many OFDM symbols does a processing stage in this mode actually contain?
    const uint32_t ps_t_length = N_step;

    /* If we are in ps_idx=0, we already have collected the entire PCC, one or two (if N_eff_TX=8)
     * DRS symbols and used the latter for channel estimation. However, we haven't collected the PDC
     * cells yet. For this reason, we go over the same symbols one more time and use the already
     * available channel estimation.
     */
    if (ps_idx == 0) {
        // reset local symbol index
        ofdm_symb_ps_idx = 0;

        // go over the same symbols again ...
        for (uint32_t i = 1; i < ofdm_symb_idx; ++i) {
            // ... and collect PDC cells
            if (pdc.is_symbol_index(i)) {
                run_pdc();
            }

            ++ofdm_symb_ps_idx;
        }
    }

    // continue at latest absolute symbol index
    for (; ofdm_symb_idx <= packet_sizes->N_DF_symb; ++ofdm_symb_idx) {
        run_mix_resample(N_b_CP_os);

        run_cp_fft_scale(N_b_CP_os);

        if (drs.is_symbol_index(ofdm_symb_idx)) {
            run_drs_chestim_zf();
            run_drs_ch_interpolation();
        }

        if (pdc.is_symbol_index(ofdm_symb_idx)) {
            run_pdc();
        }

        ++ofdm_symb_ps_idx;

        // check if the end of the processing stage was reached
        if (ofdm_symb_ps_idx == ps_t_length) {
            // update to values of next processing stage
            ++ps_idx;
            ofdm_symb_ps_idx = 0;
        }
    }
}

void rx_synced_t::run_pdc_ps_in_chestim_mode_lr_t_residual_cfo_post_correction() {
    dectnrp_assert(chestim_mode_lr, "must be called in mode lr");

    // \todo
}

void rx_synced_t::run_pdc() {
    /* Table 7.2-1: Transmission modes and transmission mode signalling defines 12 different MIMO
     * modes with index 0 to 11.
     */

    // "Effective transmission mode Single antenna": mode 0, 3 and 7
    if (sync_report->N_eff_TX == 1) {
        run_pdc_mode_single_antenna();
    } else {
        // "Effective transmission mode 2/4/8 x 1 TxDiv": mode 1, 5, and 10
        if (packet_sizes->tm_mode.N_SS == 1) {
            run_pdc_mode_transmit_diversity();
        }
        // "Effective transmission mode 2/4/8 x 2/4/8 TxDiv": mode 2, 4, 6, 8, 9, 11
        else {
            run_pdc_mode_AxA_MIMO();
        }
    }

    /* Call channel decoding, it will decode as many codeblocks as possible. If there isn't enough
     * data to decode the next codeblock, it will do nothing. When we have collected all PDC bits,
     * i.e. PDC_bits_idx==fec_cfg->G, we make the final CRC check.
     */
    fec->decode_tb(fec_cfg, *hb_tb, PDC_bits_idx);

    /* The FEC has a write pointer, which counts the number of decoded bits right after codeblock
     * segmentation. Thus, this bit counter also includes the 24 bit CRC. The mac_pdu_decoder, on
     * the other hand, expects the number of usable bytes, which at most can be the number of bytes
     * in the transport block, which does not include the transport block CRC.
     */
    mac_pdu_decoder.decode(std::min(fec->get_wp() / 8, packet_sizes->N_TB_byte));
}

void rx_synced_t::run_pdc_mode_single_antenna() {
    /* With only one transmit stream, the optimal way to combine the signals from all RX
     * antennas is MRC https://en.wikipedia.org/wiki/Maximal-ratio_combining.
     *
     * We use mixer_stage[0] to collect the numerator across all antennas for each subcarrier.
     * We use fft_stage to collect the denominator across all antennas for each subcarrier.
     */

    // load PDC subcarrier indices
    const std::vector<uint32_t>& k_i_one_symbol = pdc.get_k_i_one_symbol();
    uint32_t k_i_one_symbol_size = k_i_one_symbol.size();

    /* When the current OFDM symbol only contains PDC cells, we split processing of the symbol in
     * two steps, one for the lower and one for the upper spectrum half.
     *
     * If, however, the symbol contains not only PDC cells, we process the entire OFDM
     * symbol/spectrum in a single step and then call srsran_demod_soft_demodulate_b() and
     * srsran_demod_soft_demodulate_s() for each complex PDC cell individually. This way, internally
     * no SIMD instructions are used and the alignment is not relevant.
     */
    // two steps when only PDC cells, one step otherwise
    const uint32_t step_max = (k_i_one_symbol_size == N_b_OCC) ? 2 : 1;
    // number of cells per step: either one half the spectrum, or the full spectrum + DC carrier
    const uint32_t step_width = (step_max == 2) ? N_b_OCC / 2 : N_b_OCC + 1;

    // step by step
    for (uint32_t step = 0; step < step_max; ++step) {
        // 0 for lower spectrum, (N_b_OCC / 2 + 1) for upper
        const uint32_t read_offset = (N_b_OCC / 2 + 1) * step;

        // we reuse mixer_stage and fft_stage pointers as they are no longer required for the
        // current OFDM symbol
        srsran_vec_cf_zero(mixer_stage[0], step_width);
        srsran_vec_cf_zero(fft_stage, step_width);

        // collect from all antennas
        for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
            // numerator y*conj(h) for this antenna
            volk_32fc_x2_multiply_conjugate_32fc(
                (lv_32fc_t*)mrc_stage,
                (const lv_32fc_t*)&ofdm_symbol_now[ant_idx][read_offset],
                (const lv_32fc_t*)&channel_antennas[ant_idx]->chestim[0][read_offset],
                step_width);

            // sum up MRC numerator
            volk_32fc_x2_add_32fc((lv_32fc_t*)mixer_stage[0],
                                  (const lv_32fc_t*)mixer_stage[0],
                                  (const lv_32fc_t*)mrc_stage,
                                  step_width);

            // denominator |h|^2=h*conj(h) for this antenna
            volk_32fc_x2_multiply_conjugate_32fc(
                (lv_32fc_t*)mrc_stage,
                (const lv_32fc_t*)&channel_antennas[ant_idx]->chestim[0][read_offset],
                (const lv_32fc_t*)&channel_antennas[ant_idx]->chestim[0][read_offset],
                step_width);

            // sum up MRC denominator
            volk_32fc_x2_add_32fc((lv_32fc_t*)fft_stage,
                                  (const lv_32fc_t*)fft_stage,
                                  (const lv_32fc_t*)mrc_stage,
                                  step_width);
        }

        // MRC numerator/denomimator
        volk_32fc_x2_divide_32fc((lv_32fc_t*)mixer_stage[0],
                                 (const lv_32fc_t*)mixer_stage[0],
                                 (const lv_32fc_t*)fft_stage,
                                 step_width);

#ifdef PHY_RX_RX_SYNCED_TCP_SCOPE
        tcp_scope->send_to_scope(std::vector<cf_t*>{mixer_stage[0]}, step_width);
#endif

        // demap: all pointers must be memory-aligned!
        if (step_max == 2) {
            // entire lower of upper spectrum half
            srsran_demod_soft_demodulate_s(
                srsran_mod, mixer_stage[0], (short*)demapping_stage, step_width);

            // copy from demapping stage to d-bits of HARQ buffer
            memcpy(hb_tb->get_d(PDC_bits_idx * PHY_D_RX_DATA_TYPE_SIZE),
                   demapping_stage,
                   step_width * packet_sizes->mcs.N_bps * PHY_D_RX_DATA_TYPE_SIZE);

            PDC_subc_idx += step_width;
            PDC_bits_idx += step_width * packet_sizes->mcs.N_bps;

        } else {
            // one PDC cell at a time
            for (uint32_t i : k_i_one_symbol) {
                srsran_demod_soft_demodulate_s(
                    srsran_mod,
                    &mixer_stage[0][i],
                    (short*)hb_tb->get_d(PDC_bits_idx * PHY_D_RX_DATA_TYPE_SIZE),
                    1);

                ++PDC_subc_idx;
                PDC_bits_idx += packet_sizes->mcs.N_bps;
            }
        }
    }
}

void rx_synced_t::run_pdc_mode_transmit_diversity() {
    const std::vector<uint32_t>& k_i_one_symbol = pdc.get_k_i_one_symbol();

    // we reuse mixer_stage and fft_stage pointers as they are no longer required for the
    // current OFDM symbol
    srsran_vec_cf_zero(mixer_stage[0], k_i_one_symbol.size());
    srsran_vec_cf_zero(fft_stage, k_i_one_symbol.size());

    run_pxx_mode_transmit_diversity(k_i_one_symbol, PDC_subc_idx);

    // entire lower of upper spectrum half
    srsran_demod_soft_demodulate_s(
        srsran_mod, mixer_stage[0], (short*)demapping_stage, k_i_one_symbol.size());

    // copy from demapping stage to d-bits of HARQ buffer
    memcpy(hb_tb->get_d(PDC_bits_idx * PHY_D_RX_DATA_TYPE_SIZE),
           demapping_stage,
           k_i_one_symbol.size() * packet_sizes->mcs.N_bps * PHY_D_RX_DATA_TYPE_SIZE);

    PDC_subc_idx += k_i_one_symbol.size();
    PDC_bits_idx += k_i_one_symbol.size() * packet_sizes->mcs.N_bps;
}

void rx_synced_t::run_pdc_mode_AxA_MIMO() {
    /// \todo
}

void rx_synced_t::run_pxx_mode_transmit_diversity(const std::vector<uint32_t>& k_i_one_symbol,
                                                  const uint32_t PXX_idx) {
    // which transmit streams ared used?
    const common::vec2d<uint32_t>& index_mat_N_TS_x =
        Y_i->get_index_mat_N_TS_x(sync_report->N_eff_TX);

    const uint32_t modulo = Y_i->get_modulo(sync_report->N_eff_TX);

    dectnrp_assert(k_i_one_symbol.size() % 2 == 0, "must be even");

    // go over each RX antenna
    for (uint32_t ant_idx = 0; ant_idx < N_RX; ++ant_idx) {
        // y_mrc is the received OFDM symbol at this antenna
        const cf_t* y_mrc = ofdm_symbol_now[ant_idx];

        // collect all PCC or PDC cells
        for (uint32_t cnt = 0; cnt < k_i_one_symbol.size(); cnt += 2) {
            // indices of two neighbouring subcarriers in this OFDM symbol
            const uint32_t k_i = k_i_one_symbol[cnt];
            const uint32_t k_i_ = k_i_one_symbol[cnt + 1];

            // received values of two neighbouring subcarriers in this OFDM symbol
            const cf_t r0 = y_mrc[k_i];
            const cf_t r1 = y_mrc[k_i_];

            const uint32_t i_mod_x = ((PXX_idx + cnt) / 2) % modulo;

            const uint32_t TS_index_A = index_mat_N_TS_x[i_mod_x][0];
            const uint32_t TS_index_B = index_mat_N_TS_x[i_mod_x][1];

            // channel averaged across two subcarriers of the same transmit stream
            const cf_t h0 = (channel_antennas[ant_idx]->chestim[TS_index_A][k_i] +
                             channel_antennas[ant_idx]->chestim[TS_index_A][k_i_]) /
                            2.0f;
            const cf_t h1 = (channel_antennas[ant_idx]->chestim[TS_index_B][k_i] +
                             channel_antennas[ant_idx]->chestim[TS_index_B][k_i_]) /
                            2.0f;

            const cf_t h0_conj = conjj(h0);
            const cf_t h1_conj = conjj(h1);

            // MRC numerator: y*conj(h)
            mixer_stage[0][cnt] += h0_conj * r0 + h1 * conjj(r1);
            mixer_stage[0][cnt + 1] += -h1 * conjj(r0) + h0_conj * r1;

            // MRC denominator: |h|^2 = h*conj(h)
            const cf_t abs_h_square = h0 * h0_conj + h1 * h1_conj;
            fft_stage[cnt] += abs_h_square;
            fft_stage[cnt + 1] += abs_h_square;
        }
    }

    // MRC numerator/denomimator (technically not required for PCC as PCC always uses QPSK)
    volk_32fc_x2_divide_32fc((lv_32fc_t*)mixer_stage[0],
                             (const lv_32fc_t*)mixer_stage[0],
                             (const lv_32fc_t*)fft_stage,
                             k_i_one_symbol.size());
}

}  // namespace dectnrp::phy
