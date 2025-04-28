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

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/common/json/json_switch.hpp"
#include "dectnrp/phy/harq/buffer_rx.hpp"
#include "dectnrp/phy/harq/buffer_rx_plcf.hpp"
#include "dectnrp/phy/interfaces/maclow_phy.hpp"
#include "dectnrp/phy/mix/mixer.hpp"
#include "dectnrp/phy/rx/rx_pacer.hpp"
#include "dectnrp/phy/rx/rx_synced/aoa/estimator_aoa.hpp"
#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_antennas.hpp"
#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_luts.hpp"
#include "dectnrp/phy/rx/rx_synced/mimo/estimator_mimo.hpp"
#include "dectnrp/phy/rx/rx_synced/offsets/estimator_cfo.hpp"
#include "dectnrp/phy/rx/rx_synced/offsets/estimator_sto.hpp"
#include "dectnrp/phy/rx/rx_synced/pcc_report.hpp"
#include "dectnrp/phy/rx/rx_synced/pdc_report.hpp"
#include "dectnrp/phy/rx/rx_synced/processing_stage.hpp"
#include "dectnrp/phy/rx/rx_synced/snr/estimator_snr.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"
#include "dectnrp/phy/tx_rx.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_pdu_decoder.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_decoder.hpp"

// #define PHY_RX_RX_SYNCED_TCP_SCOPE
#ifdef PHY_RX_RX_SYNCED_TCP_SCOPE
#include "dectnrp/common/adt/tcp_scope.hpp"
#endif

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
#include "dectnrp/external/nlohmann/json.hpp"
#endif

namespace dectnrp::phy {

class rx_synced_t final : public tx_rx_t, public rx_pacer_t {
    public:
        /**
         * \brief Demodulates and decodes packets with known synchronization in time and frequency
         * domain.
         *
         * \param buffer_rx_ access to received IQ samples
         * \param worker_pool_config_ configuration of worker pool this class is part of
         * \param ant_streams_unit_length_samples_ maximum number of samples per resampler call
         */
        explicit rx_synced_t(const radio::buffer_rx_t& buffer_rx_,
                             const worker_pool_config_t& worker_pool_config_,
                             const uint32_t ant_streams_unit_length_samples_);
        ~rx_synced_t();

        rx_synced_t() = delete;
        rx_synced_t(const rx_synced_t&) = delete;
        rx_synced_t& operator=(const rx_synced_t&) = delete;
        rx_synced_t(rx_synced_t&&) = delete;
        rx_synced_t& operator=(rx_synced_t&&) = delete;

        /**
         * \brief First, try to decode the information from the PCC. We blindly try both PLCF type 1
         * and type 2, run a check to see if they are within the boundaries set by the radio device
         * class, and interpret the bits. This information is used in worker_tx_rx to determine
         * whether we want to proceed with the PDC and, if so, which HARQ buffer to use. This
         * decision depends on the network ID, transmitter/receiver identity etc. and must be made
         * on lower MAC level.
         *
         * \param sync_report_ report produced by one of the workers for sync
         * \return success status and content of PCC decoding
         */
        pcc_report_t demoddecod_rx_pcc(sync_report_t& sync_report_);

        /**
         * \brief If, based on PCC, the decision was made to decode the PDC as well, this function
         * must be called. The state of rx_synced_t is not to be changed so that we can seamlessly
         * continue with demodulation, channel estimation, decoding etc.
         *
         * \param maclow_phy_ instructions from lower MAC to PHY
         * \return success status and content of PDC decoding
         */
        pdc_report_t demoddecod_rx_pdc(const maclow_phy_t& maclow_phy_);

        /**
         * \brief Before starting a new decoding process, the internal buffers of the HARQ buffer
         * for PCC have to be reset. This function must be called whether PCC was decoded
         * successfully or not.
         */
        void reset_for_next_pcc();

#ifdef PHY_JSON_SWITCH_IMPLEMENT_ANY_JSON_FUNCTIONALITY
        nlohmann::ordered_json get_json() const;
#endif

    private:
        // ##################################################
        // RX synced specific variables initialized once in the constructor

        /**
         * \brief At the receiver, we make the following assumption about the number of receive
         * antennas: N_TX = N_RX, i.e. all physical antennas used for TX (number defined by the
         * radio device class) are also used for RX. This assumption has also been made in
         * sync_chunk.cpp, where we use all antennas for synchronization.
         */
        const uint32_t N_RX;

        /// read-only container for resampler output
        const std::vector<cf_t*> localbuffer_resample;

        /// What is a stage? see tx.cpp
        std::vector<cf_t*> mixer_stage;
        cf_t* fft_stage{};
        cf_t* mrc_stage{};
        uint8_t* demapping_stage{};

        /**
         * \brief A processing stage is defined as a sequence of OFDM symbols enclosed to its left
         * and right by OFDM symbols containing DRS cells used for channel estimation. Its length
         * depends on N_eff_TX which is the same as the number of transmit streams. Looking at
         * Figure 4.5-2 a) to c) as well as Figure 4.5-3 d) and e), the possible lengths are:
         *
         * 1. N_eff_TX = 1, N_Step =  5, processing_stage_len = 6
         * 2. N_eff_TX = 2, N_Step =  5, processing_stage_len = 6
         * 3. N_eff_TX = 4, N_Step = 10, processing_stage_len = 11
         * 4. N_eff_TX = 8, N_Step = 10, processing_stage_len = 12
         */
        std::unique_ptr<processing_stage_t<cf_t>> processing_stage;

        /// conversion LUT for ps length
        static constexpr uint32_t N_eff_TX2processing_stage_len[9] = {0, 6, 6, 0, 11, 0, 0, 0, 12};

        /// various estimators for optimal decoding
        std::unique_ptr<estimator_sto_t> estimator_sto;
        std::unique_ptr<estimator_cfo_t> estimator_cfo;
        std::unique_ptr<estimator_snr_t> estimator_snr;
        std::unique_ptr<estimator_mimo_t> estimator_mimo;
        std::unique_ptr<estimator_aoa_t> estimator_aoa;

        /**
         * \brief Every channel estimation is conducted within one processing stage in one of two
         * modes:
         *
         * 1. chestim_mode_lr = true: Within each processing stage, the channel estimate at every
         * subcarrier depends on DRS symbols on the left AND the right side (lr) of the processing
         * stage. This mode offers better channel estimates, but we have to wait for the entire
         * processing stage to be collected before we can start estimating the channel. Also, this
         * mode isn't always applicable, for instance when a frame ends with an OFDM symbol without
         * DRS cells.
         *
         * 2. chestim_mode_lr = false: Within each processing stage, the channel estimate at every
         * subcarrier depends only on DRS cells on the left side of the processing stage. This mode
         * is computationally less complex and causes less latency, since we can retrieve channel
         * estimates before having collected the entire processing stage. However, the channel
         * estimates are not as precise as for chestim_mode_lr = true.
         */
        const bool chestim_mode_lr_default;  // default mode used whenever possible

        /**
         * \brief When chestim_mode_lr = true, we can interpolate at every OFDM symbol index within
         * the processing stage, which gives us the best channel estimate. However, this is also
         * computationally very complex and causes latency.
         *
         * As a solution, we define a stride. We interpolate at the first symbol index 0, then use
         * that estimate for the next couple of symbols, until we refresh the channel estimate at
         * symbol index 0+chestim_mode_lr_t_stride. The minimum stride is 1.
         */
        const uint32_t chestim_mode_lr_t_stride;

        /**
         * \brief The process of channel estimation for any RX antenna is always the same, i.e. it
         * does not depend on the RX antenna index. Thus, when estimating the channel at any RX
         * antenna, the same vector element from channel_luts is used. Every vector element is
         * optimized for different channel conditions and contains interpolation, extrapolation and
         * smoothing parameters.
         */
        channel_luts_t channel_luts;
        channel_lut_t* channel_lut_effective{};

        /**
         * \brief The process of channel estimation generates one channel estimate per RX antenna,
         * which is saved in the vector channel_antennas. Thus, this vector's length is the same as
         * the number of RX antennas.
         *
         * The process of channel estimation is the following:
         *
         * 1. Each of the N_RX receive antennas has received a superposition of TX signals from all
         * N_eff_TX transmit streams.
         *
         * 2. For each RX antenna, collect time domain samples of a particular OFDM symbol, drop the
         * CP and execute the FFT. This yields N_RX many vectors in frequency domain.
         *
         * 3. For each RX antenna's post-FFT vector in frequency domain ...
         *
         * 3.1. Go over each N_eff_TX transmit stream, estimate the channel at the DRS cells by
         * zero-forcing, and write these values consecutively into channel_antennas. Going over all
         * transmit streams in a single step has the benefit of reading all vectors consecutively,
         * which (presumably) is cache-friendly.
         *
         * 3.2. Go over each N_eff_TX transmit stream and use the consecutive zero-forced channel
         * estimates from step 3.1 to interpolate, extrapolate and smooth the channel at all the
         * other subcarriers of the OFDM symbol in frequency domain. Write the result to
         * channel_antennas. Going over all transmit streams in a single step has the benefit of
         * reading all vectors consecutively, which (presumably) is cache-friendly.
         */
        channel_antennas_t channel_antennas;

        /**
         * \brief After collecting and demapping all PCC cells, we decode both PLCF type 1 and type
         * 2 and check the CRC for both versions. For this, we use the variable hb_rx_plcf which
         * contains one a-bit buffer and one d-bit buffer, but two softbuffers PLCF type 1 and
         * type 2. Simultaneously, we extract the masking configuration of CL and BF that any
         * correct CRC had. After that, we interpret the decoded PLCFs within the plcf_pool.
         */
        std::unique_ptr<harq::buffer_rx_plcf_t> hb_rx_plcf;
        std::unique_ptr<section4::plcf_decoder_t> plcf_decoder;

        /**
         * \brief If the lower MAC has made the decision to continue with the PDC, the function
         * demoddecod_rx_pdc() is called which decodes codeblock for codeblock. Simultaneously, we
         * interpret the bits, i.e. the MAC messages and information elements. For this, we use this
         * mac_pdu_decoder.
         */
        section4::mac_pdu_decoder_t mac_pdu_decoder;

        // ##################################################
        // RX synced specific variables updated for every new packet

        /// make sync report from sync pool accessible to all functions
        const sync_report_t* sync_report{};

        /// values refer to localbuffer_resample
        uint32_t localbuffer_cnt_w;  // nof samples written by resampler
        uint32_t localbuffer_cnt_r;  // nof samples already processed

        /// used for correction of CFO, is reconfigured for every new packet
        mixer_t mixer;

        /// channel estimation mode used for current processing stage
        bool chestim_mode_lr;
        /// first processing stage always has index 0 (even), then 1, 2 ...
        uint32_t ps_idx;
        /// the length of a processing stage in OFDM symbols (t-domain) depends on
        /// chestim_mode_lr
        uint32_t N_step;
        /// during channel estimation, we save the current processable transmit stream
        /// indices
        uint32_t TS_idx_first, TS_idx_last;

        /**
         * \brief While tx_rx_t::ofdm_symb_idx is an absolute OFDM symbol index within the packet,
         * this variable represents the OFDM symbol index relative within the current processing
         * stage.
         */
        uint32_t ofdm_symb_ps_idx;
        /// pointer to current OFDM symbol in processing stage, one element per RX antenna
        std::vector<cf_t*> ofdm_symbol_now;

        /// before decoding the PDC, we have to configure the FEC in this structure
        section3::fec_cfg_t fec_cfg;

        /// readability pointer to elements on maclow_phy
        const maclow_phy_t* maclow_phy;
        const section3::packet_sizes_t* packet_sizes;
        harq::buffer_rx_t* hb_tb;

        /// demapper type
        srsran_mod_t srsran_mod;

        // ##################################################
        // RX synced specific functions

        void run_symbol_dimensions();

        void run_stf(sync_report_t& sync_report_);
        void run_stf_rms_estimation(sync_report_t& sync_report_);
        void run_stf_chestim_zf();

        /**
         * \brief Called for every OFDM symbol to resample and correct CFO directly onto
         * mixer_stage. The current length of the cyclic prefix is given as an argument since this
         * function can be used for the STF and data field (DF), see section 5.1 in part 3. STF has
         * a longer cyclic prefix.
         *
         * In the case of the STF, once the samples are put onto mixer_stage, we can revert the
         * cover sequence. That is also the reason why both functions run_mix_resample() and
         * run_cp_fft_scale() are required.
         */
        void run_mix_resample(const uint32_t N_samples_in_CP_os);

        /**
         * \brief Called for every OFDM symbol to drop CP, transform into frequency domain and
         * remove OS carrier. Output is written onto processing_stage. The current length of the
         * cyclic prefix is given as an argument since this function can be used for the STF and
         * data field (DF), see section 5.1 in part 3. STF has a longer cyclic prefix.
         */
        void run_cp_fft_scale(const uint32_t N_samples_in_CP_os);

        /// called whenever an OFDM symbol contains DRS cells, zf stands for zero-forcing
        void run_drs_chestim_zf();
        /**
         * \brief find optimal Wiener filter for current channel estimation, can only be called when
         * a valid SNR estimation is available
         */
        void run_drs_channel_lut_pick();
        /**
         * \brief Called when channel estimation can be started. If chestim_mode_lr=true, the entire
         * processing stage with DRS symbols to the left and right has to be collected first. If
         * chestim_mode_lr=false, function must be called right after run_drs_chestim_zf().
         */
        void run_drs_ch_interpolation();

        /// collect PCC cells from current OFDM symbol
        void run_pcc_collection_and_demapping();
        /**
         * \brief After demapping 98 complex QPSK symbols to 196 bits, decode them, check the CRC
         * and the correctness of the values.
         */
        void run_pcc_decoding_and_candidate_search();

        /**
         * \brief While the PCC is always processed in mode lr=false, we make a distinction between
         * how processing stages are processed for PDC. t=true, f=false
         */
        void run_pdc_ps_in_chestim_mode_lr_t();
        void run_pdc_ps_in_chestim_mode_lr_f();

        /**
         * \brief When processing the PDC in mode_lr=true, DRS symbols are collected to the left and
         * right of the processing stage which gives us the best possible residual CFO estimation.
         * Immediately after obtaining the right DRS symbol, the mixer's phase rotation is adjusted.
         *
         * However, at that point mixing and the FFT for all symbol in between the DRS symbols have
         * already been executed, so the mixer adjustment can't take effect. As a solution, we have
         * rotation the phase of all symbols in frequency domain.
         */
        void run_pdc_ps_in_chestim_mode_lr_t_residual_cfo_post_correction();

        /**
         * \brief Collect PDC and extract binary information. Speaking in classical OFDM receiver
         * terminology, the components up to the channel estimation are part of the inner receiver.
         * Extract and decoding the PDC, however, is part of the outer receiver.
         */
        void run_pdc();
        void run_pdc_mode_single_antenna();
        void run_pdc_mode_transmit_diversity();
        void run_pdc_mode_AxA_MIMO();

        // same function can be reused for PCC and PDC
        void run_pxx_mode_transmit_diversity(const std::vector<uint32_t>& k_i_one_symbol,
                                             const uint32_t PXX_idx);

        // ##################################################
        // RX synced specific statistics

        // RX synced does not keep statistics

#ifdef PHY_RX_RX_SYNCED_TCP_SCOPE
        std::unique_ptr<common::adt::tcp_scope_t<cf_t>> tcp_scope;
#endif
};

}  // namespace dectnrp::phy
