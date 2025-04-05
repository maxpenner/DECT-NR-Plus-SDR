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

extern "C" {
#include "srsran/phy/modem/modem_table.h"
}

#include "dectnrp/phy/mix/mixer.hpp"
#include "dectnrp/phy/tx/tx_descriptor.hpp"
#include "dectnrp/phy/tx_rx.hpp"
#include "dectnrp/radio/buffer_tx.hpp"
#include "dectnrp/sections_part3/beamforming_and_antenna_port_mapping.hpp"

#define PHY_TX_FEC_OF_PDC_CODEBLOCK_FOR_CODEBLOCK_OR_COMPLETE_AT_THE_BEGINNING
#define PHY_TX_BACKPRESSURED_OR_PACKET_IS_ALWAYS_COMPLETE
#define PHY_TX_PCC_FLIPPING_WITH_SIMD

// #define PHY_TX_OFDM_WINDOWING 0.25f
#ifdef PHY_TX_OFDM_WINDOWING
#include "dectnrp/phy/dft/windowing.hpp"
#endif

/* When debugging with JSON files, backpressure can't be active. Function write_all_data_to_json()
 * calls get_ant_streams(), but buffer is already flagged as ready for transmission. This triggers
 * an assert.
 */
// #define PHY_TX_JSON_EXPORT
#ifdef PHY_TX_JSON_EXPORT
#undef PHY_TX_BACKPRESSURED_OR_PACKET_IS_ALWAYS_COMPLETE
#endif

namespace dectnrp::phy {

class tx_t final : public tx_rx_t {
    public:
        /**
         * \brief Generates packets for transmission
         *
         * \param maximum_packet_sizes_ maximum sizes defined by radio device class
         * \param os_min_ minimum oversampling for largest bandwidth
         * \param resampler_param_ resampler configuration
         */
        explicit tx_t(const section3::packet_sizes_t maximum_packet_sizes_,
                      const uint32_t os_min_,
                      const resampler_param_t resampler_param_);
        ~tx_t();

        tx_t() = delete;
        tx_t(const tx_t&) = delete;
        tx_t& operator=(const tx_t&) = delete;
        tx_t(tx_t&&) = delete;
        tx_t& operator=(tx_t&&) = delete;

        /**
         * \brief tx_descriptor_t contains everything we need to generate a packet into buffer_tx_t
         *
         * \param tx_descriptor_ instructions from MAC to PHY
         * \param buffer_tx_ access to IQ samples read by radio layer
         */
        void generate_tx_packet(const tx_descriptor_t& tx_descriptor_,
                                radio::buffer_tx_t& buffer_tx_);

    private:
        // ##################################################
        // TX specific variables initialized once in the constructor

        /// BPSK to 256-QAM (1024-QAM not implemented)
        std::array<srsran_modem_table_t, 5> srsran_modem_table;

#ifdef PHY_TX_OFDM_WINDOWING
        std::array<dft::windowing_t, N_fft_sizes> windowing_array;
#endif

        /// beamforming
        section3::W_t W;

        /// resampler and resampling ratio do not change
        std::unique_ptr<resampler_t> resampler;

        /// used to frequency shift entire signal
        mixer_t mixer;

        /**
         * \brief What is a stage?
         *
         * The individual processing steps of TX are shown in Figure 7.1-1 in part 3. The results of
         * each step are copied onto the respective stage, e.g. subcarriers being transmit diversity
         * coded are copied onto the transmit_streams_stage.
         *
         * Stage for spatial streams is not required.
         * When N_SS is 1 (SISO), we can directly copy into transmit_streams_stage.
         * When N_SS is >1 (MIMO), we can directly copy into transmit_streams_stage with a stride.
         *
         * \todo: Can SIMD instructions be used to copy with stride?
         */
        std::vector<cf_t*> transmit_streams_stage;

        /**
         * \brief Each antenna signal is a superposition of differently weighted transmit streams.
         * The beamforming stage stores the multiplication of one transmit stream with a specific
         * weight, which is then added to the antenna_mapper_stage. When the product of a specific
         * TS and a specific weight is added to multiple antenna streams, it is only calculated
         * once, see tx.cpp for details.
         */
        cf_t* beamforming_stage;
        std::vector<cf_t*> antenna_mapper_stage;

        /// stage in time domain, resampler writes directly into buffer_tx_
        std::vector<cf_t*> ifft_cp_stage;

        // ##################################################
        // TX specific variables updated for every new packet

        /// readability pointer to elements of tx_descriptor_t
        const tx_descriptor_t* tx_descriptor;
        const section3::packet_sizes_t* packet_sizes;
        const tx_meta_t* tx_meta;
        harq::buffer_tx_t* hb_plcf;
        harq::buffer_tx_t* hb_tb;

        /// readability pointer to buffer_tx reference
        radio::buffer_tx_t* buffer_tx;

        /// pointer to antennas ports/streams
        std::vector<cf_t*> antenna_ports;
        std::vector<cf_t*> antenna_ports_now;

#ifdef PHY_TX_JSON_EXPORT
        double oversampling;

        /// save meta, binary and IQ data of frame on drive
        void write_all_data_to_json() const;
#endif

        /// channel coding
        section3::fec_cfg_t fec_cfg;

        /// depends on MCS
        srsran_modem_table_t* srsran_modem_table_effective;

        /// packet lengths with and without GI
        uint32_t N_samples_packet_no_GI_os;     // STF + DF oversampled
        uint32_t N_samples_packet_no_GI_os_rs;  // STF + DF oversampled and resampled with tail
                                                // samples from resampler
        uint32_t N_samples_transmit_os_rs;      // final amount of samples we transmit after
                                                // oversampling and resampling, with or without GI

        /// ETSI TS 103 636-3 V1.3.1 (2021-12), 6.3.5 OFDM signal generation
        float final_scale_STF;
        float final_scale;

        /// internal counters that are checked after packet generation
        uint32_t index_sample_no_GI_os;
        uint32_t index_sample_transmit_os_rs;

        // ##################################################
        // TX specific functions

        /// called once per packet
        void run_packet_dimensions();
        void run_meta_dependencies();
        void run_pcc_symbol_mapper_and_flipper();
        void run_residual_resampling();
        void run_GI();

        /// called for each OFDM symbol (arguments are necessary to distinguish STF from the other
        /// symbols)
        void run_zero_stages();
        void run_beamforming(const uint32_t N_TS_non_zero);
        void run_ifft_cp_scale(const uint32_t N_samples_in_CP_os, float scale);
        void run_resampling_and_freq_shift(const uint32_t N_samples_in_CP_os);

        /// called for each OFDM symbol which contains the respective type of subcarriers
        void run_stf();
        void run_pcc();
        void run_drs();
        void run_pdc();

        // ##################################################
        // TX specific statistics

        // TX does not keep statistics
};

}  // namespace dectnrp::phy
