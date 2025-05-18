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

#include "dectnrp/phy/tx_rx.hpp"

#include <algorithm>
#include <limits>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/phy/resample/resampler.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::phy {

tx_rx_t::tx_rx_t(const section3::packet_sizes_t maximum_packet_sizes_,
                 const uint32_t os_min_,
                 const resampler_param_t resampler_param_)
    : maximum_packet_sizes(maximum_packet_sizes_),
      os_min(os_min_),
      dect_samp_rate_oversampled_max(maximum_packet_sizes.psdef.u * maximum_packet_sizes.psdef.b *
                                     constants::samp_rate_min_u_b * os_min),
      resampler_param(resampler_param_) {
    dectnrp_assert(maximum_packet_sizes.mcs.N_bps <= limits::dectnrp_max_mcs_index,
                   "MCS not supported");

    // determine the sample rate after resampling
    const uint32_t apparent_samp_rate_after_resampling =
        resampler_t::get_samp_rate_converted_with_temporary_overflow(
            dect_samp_rate_oversampled_max, resampler_param.L, resampler_param.M);

    // if we are not using resampling
    if (resampler_param.L == 1 && resampler_param.M == 1) {
        dectnrp_assert(
            apparent_samp_rate_after_resampling <= resampler_param.hw_samp_rate,
            "resampling off, but hardware sample rate smaller than maximum DECT NR+ sample rate.");
    } else {
        dectnrp_assert(
            apparent_samp_rate_after_resampling == resampler_param.hw_samp_rate,
            "resampling on, but hardware sample rate not exactly maximum DECT NR+ sample rate.");
    }

    const uint32_t b_max = maximum_packet_sizes.psdef.b;
    const uint32_t b_idx_max = section3::phyres::b2b_idx[b_max];
    const uint32_t N_b_OCC_max = section3::phyres::N_b_OCC_lut[b_idx_max];
    const uint32_t N_TS_max = maximum_packet_sizes.tm_mode.N_TS;

    // N_eff_TX_max = N_TS_max
    stf = section3::stf_t(b_max, N_TS_max, 1.0f);
    pcc = section3::pcc_t(b_max, N_TS_max);
    drs = section3::drs_t(b_max, N_TS_max);
    pdc = section3::pdc_t(b_max, N_TS_max);
    Y_i = std::make_unique<section3::Y_i_t>(N_b_OCC_max, N_TS_max);

    // fec uses a lot of RAM for large lookup tables
    fec = std::make_unique<fec_t>(maximum_packet_sizes);

    // what is the largest FFT size required?
    const auto fft_size_max_required =
        dect_samp_rate_oversampled_max / constants::subcarrier_spacing_min_u_b;

    dectnrp_assert(std::find(fft_sizes_all.begin(), fft_sizes_all.end(), fft_size_max_required) !=
                       fft_sizes_all.end(),
                   "unknown FFT size");

    const auto N_fft_sizes_required =
        std::find(fft_sizes_all.begin(), fft_sizes_all.end(), fft_size_max_required) -
        fft_sizes_all.begin() + 1;

    ofdm_vec.resize(N_fft_sizes_required);

    dectnrp_assert(ofdm_vec.size() > 0, "no FFT size defined");

    // always init every fft size
    for (uint32_t i = 0; i < ofdm_vec.size(); ++i) {
        dft::get_ofdm(ofdm_vec[i], fft_sizes_all[i]);
    }

    // PCC always has 196/2=98 complex QPSK symbols
    pcc_qpsk_symbols = srsran_vec_cf_malloc(constants::pcc_cells);
    pcc_qpsk_symbols_flipped = srsran_vec_cf_malloc(constants::pcc_cells);

    /* FEC gives us packed bytes after channel coding. We need to take these packed bytes and turn
     * them into complex BPSK/QPSK/etc. symbols for PDC. For a given OFDM symbol, the amount of
     * complex values a.k.a. subcarriers required is N_SS * N_b_OCC. The equivalent amount of bits
     * is N_SS * N_b_OCC * N_bps.
     *
     * Unfortunately, N_SS * N_b_OCC * N_bps is not necessarily a multiple of 8, but our input bytes
     * are packed and we want to process full bytes only.
     *
     * As a solution, we process only multiples of 24 bits (3 bytes), i.e. ceil(N_SS * N_b_OCC *
     * N_bps/24) * 24 (Exception: For the last OFDM symbol in a packet it may not be a multiple of
     * 24). See run_pdc().
     *
     * We are guaranteed that the amount of bits is a multiple of 8 and therefore a full byte.
     * We are guaranteed that the amount of bits is a multiple of any value of N_bps = 1/2/4/6/8
     * (but not 10, least common mutiples would be 120, but 1024 QAM not implemented anyway).
     *
     * For an OFDM symbol we might generate more complex symbols than required, these are then used
     * in the next symbol. In the worst case, we need k*24 + 1 bits for BPSK. The next multiple of
     * 24 is (k*24 + 1 + 23), so we generate an additional 23 complex symbols.
     */
    pdc_cmplx_symbols = srsran_vec_cf_malloc(
        maximum_packet_sizes.tm_mode.N_SS * maximum_packet_sizes.numerology.N_b_OCC + 23);
    pdc_cmplx_symbols_flipped = srsran_vec_cf_malloc(
        maximum_packet_sizes.tm_mode.N_SS * maximum_packet_sizes.numerology.N_b_OCC + 23);
}

tx_rx_t::~tx_rx_t() {
    for (auto& elem : ofdm_vec) {
        dft::free_ofdm(elem);
    }

    free(pcc_qpsk_symbols);
    free(pcc_qpsk_symbols_flipped);

    free(pdc_cmplx_symbols);
    free(pdc_cmplx_symbols_flipped);
}

void tx_rx_t::add_new_network_id(const uint32_t network_id) { fec->add_new_network_id(network_id); }

void tx_rx_t::set_ofdm_vec_idx_effective(const uint32_t N_b_DFT_os_) {
    ofdm_vec_idx_effective = std::numeric_limits<uint32_t>::max();
    switch (N_b_DFT_os_) {
        case 64:
            ofdm_vec_idx_effective = 0;
            break;
        case 128:
            ofdm_vec_idx_effective = 1;
            break;
        case 256:
            ofdm_vec_idx_effective = 2;
            break;
        case 512:
            ofdm_vec_idx_effective = 3;
            break;
        case 768:
            ofdm_vec_idx_effective = 4;
            break;
        case 1024:
            ofdm_vec_idx_effective = 5;
            break;
        case 1536:
            ofdm_vec_idx_effective = 6;
            break;
        case 2048:
            ofdm_vec_idx_effective = 7;
            break;
        case 3072:
            ofdm_vec_idx_effective = 8;
            break;
        case 4096:
            ofdm_vec_idx_effective = 9;
            break;
        case 6144:
            ofdm_vec_idx_effective = 10;
            break;
        case 8192:
            ofdm_vec_idx_effective = 11;
            break;
        case 12288:
            ofdm_vec_idx_effective = 12;
            break;
        case 16384:
            ofdm_vec_idx_effective = 13;
            break;
    }

    dectnrp_assert(ofdm_vec_idx_effective < std::numeric_limits<uint32_t>::max(),
                   "undefined FFT length");
    dectnrp_assert(ofdm_vec_idx_effective < ofdm_vec.size(), "FFT length out-of-bound");
}

void tx_rx_t::set_N_subc_offset_lower_half_os(const uint32_t N_b_DFT_os_,
                                              const uint32_t N_b_DFT_,
                                              const uint32_t N_guards_bottom_) {
    /* Offset needed for FFT mirror.
     *           _
     *          |
     *          |   Guards OS
     *          |
     *          |_
     *          |_  Guards Top
     *          |
     *          |   N_b_DFT/2 minus Guards Top carriers
     *          |_
     *          |
     *          |   N_b_DFT/2 minus Guards Bottom carriers
     *          |_
     *          |_  Guards Bottom
     *          |
     *          |
     *          |   Guards OS
     *          |_
     *
     *      after mirroring:
     *           _
     *          |
     *          |   N_b_DFT/2 minus Guards Bottom carriers
     *          |_
     *          |_  Guards Bottom
     *          |
     *          |
     *          |   Guards OS
     *          |_
     *          |
     *          |   Guards OS
     *          |
     *          |_
     *          |_  Guards Top
     *          |
     *          |   N_b_DFT/2 minus Guards Top carriers
     *          |_
     */
    const uint32_t N_b_DFT_guard_os = (N_b_DFT_os_ - N_b_DFT_) / 2;
    N_subc_offset_lower_half_os = N_b_DFT_ / 2 + 2 * N_b_DFT_guard_os + N_guards_bottom_;
}

void tx_rx_t::reconfigure_packet_components(const uint32_t b, const uint32_t N_TS_or_N_eff_TX) {
    // reconfigure state machines
    pcc.set_configuration(b, N_TS_or_N_eff_TX);
    drs.set_configuration(b, N_TS_or_N_eff_TX);
    pdc.set_configuration(b, N_TS_or_N_eff_TX);
}

void tx_rx_t::reset_common_counters() {
    ofdm_symb_idx = 0;
    PCC_idx = 0;
    DRS_idx = 0;
    PDC_bits_idx = 0;
    PDC_subc_idx = 0;
    PDC_nof_cmplx_subc_residual = 0;
}

}  // namespace dectnrp::phy
