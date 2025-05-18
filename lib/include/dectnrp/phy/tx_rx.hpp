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

#include <array>
#include <memory>
#include <vector>

#include "dectnrp/common/complex.hpp"
#include "dectnrp/phy/dft/ofdm.hpp"
#include "dectnrp/phy/fec/fec.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"
#include "dectnrp/sections_part3/drs.hpp"
#include "dectnrp/sections_part3/pcc.hpp"
#include "dectnrp/sections_part3/pdc.hpp"
#include "dectnrp/sections_part3/stf.hpp"
#include "dectnrp/sections_part3/transmit_diversity_precoding.hpp"

namespace dectnrp::phy {

class tx_rx_t {
    public:
        virtual ~tx_rx_t();

        tx_rx_t() = delete;
        tx_rx_t(const tx_rx_t&) = delete;
        tx_rx_t& operator=(const tx_rx_t&) = delete;
        tx_rx_t(tx_rx_t&&) = delete;
        tx_rx_t& operator=(tx_rx_t&&) = delete;

        /// network IDs are used on PHY for scrambling and therefore must be precalculated,
        /// otherwise timing can hiccup
        void add_new_network_id(const uint32_t network_id);

    protected:
        /**
         * \brief Abstract base class for tx and rx.
         *
         * \param maximum_packet_sizes_
         * \param os_min_
         * \param resampler_param_
         */
        explicit tx_rx_t(const section3::packet_sizes_t maximum_packet_sizes_,
                         const uint32_t os_min_,
                         const resampler_param_t resampler_param_);

        /// enough for 221.184MHz with oversampling=2, since 221.184Mhz*2/27kHz = 16384
        static constexpr uint32_t N_fft_sizes_all{14};
        static constexpr std::array<uint32_t, N_fft_sizes_all> fft_sizes_all{
            64, 128, 256, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384};

        // ##################################################
        // TX/RX variables initialized once in the constructor

        const section3::packet_sizes_t maximum_packet_sizes;
        const uint32_t os_min;
        const uint32_t dect_samp_rate_oversampled_max;

        /// connection between DECT sample rate and hw sample rate
        const resampler_param_t resampler_param;

        section3::stf_t stf;  // constructor creates LUTs, must be loaded for every STF
        section3::pcc_t pcc;  // constructor creates LUTs, must be reconfigured for every packet
        section3::drs_t drs;  // constructor creates LUTs, must be reconfigured for every packet
        section3::pdc_t pdc;  // constructor creates LUTs, must be reconfigured for every packet

        /// transmit diversity coding, constructor creates LUTs, must be loaded for each symbol
        std::unique_ptr<section3::Y_i_t> Y_i;

        /// initialized in constructor, must be reconfigured for every new packet
        std::unique_ptr<fec_t> fec;

        /// We offer multiple different IFFT sizes. This way we can oversample every possible signal
        /// bandwidth and subcarrier spacing to dect_samp_rate_oversampled_max. A suitable IFFT size
        /// is picked for every packet.
        std::vector<dft::ofdm_t> ofdm_vec;

        /// will have a size of 196/2 = 98 complex symbols
        cf_t* pcc_qpsk_symbols;
        cf_t* pcc_qpsk_symbols_flipped;

        /// complex pdc symbols
        cf_t* pdc_cmplx_symbols;
        cf_t* pdc_cmplx_symbols_flipped;

        // ##################################################
        // TX/RX variables updated for every new packet

        uint32_t N_b_DFT_os;
        uint32_t ofdm_vec_idx_effective;
        uint32_t N_b_DFT;
        uint32_t N_b_OCC;
        uint32_t N_b_OCC_plus_DC;
        uint32_t N_subc_offset_lower_half_os;

        /// cyclic prefix lengths with oversampling but without resampling
        uint32_t N_samples_STF_CP_only_os;
        uint32_t N_b_CP_os;

        /// see Table 7.2-1 in part 3
        section3::tmmode::tm_mode_t tm_mode;
        bool transmit_diversity_mode;

        /// internal counters
        uint32_t ofdm_symb_idx;
        uint32_t PCC_idx;
        uint32_t DRS_idx;
        uint32_t PDC_bits_idx;
        uint32_t PDC_subc_idx;
        uint32_t PDC_nof_cmplx_subc_residual;

        // ##################################################
        // TX/RX functions

        void set_ofdm_vec_idx_effective(const uint32_t N_b_DFT_os_);

        void set_N_subc_offset_lower_half_os(const uint32_t N_b_DFT_os_,
                                             const uint32_t N_b_DFT_,
                                             const uint32_t N_guards_bottom_);

        void reconfigure_packet_components(const uint32_t b, const uint32_t N_TS_or_N_eff_TX);

        void reset_common_counters();
};

}  // namespace dectnrp::phy
