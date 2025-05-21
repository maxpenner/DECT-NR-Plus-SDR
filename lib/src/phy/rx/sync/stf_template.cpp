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

#include "dectnrp/phy/rx/sync/stf_template.hpp"

#include <cmath>
#include <memory>
#include <vector>

extern "C" {
#include "srsran/phy/utils/vector.h"
#include "srsran/phy/utils/vector_simd.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/dft/ofdm.hpp"
#include "dectnrp/phy/resample/resampler.hpp"
#include "dectnrp/phy/rx/sync/sync_param.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

namespace dectnrp::phy {

stf_template_t::stf_template_t(const uint32_t u_max_,
                               const uint32_t b_max_,
                               const uint32_t os_min_,
                               const uint32_t N_eff_TX_max_,
                               const resampler_param_t resampler_param_)
    : stf_bos_length_samples(sp3::stf_t::get_N_samples_stf(u_max_) * b_max_ * os_min_),
      stf_bos_rs_length_samples(stf_bos_length_samples * resampler_param_.L / resampler_param_.M),
      stf_bos_rs_length_effective_samples(
          static_cast<uint32_t>(static_cast<double>(stf_bos_rs_length_samples) *
                                RX_SYNC_PARAM_CROSSCORRELATOR_STF_LENGTH_EFFECTIVE_DP)),
      stf(sp3::stf_t(b_max_, N_eff_TX_max_, 1.0f)),
      stf_time_domain(generate_stf_time_domain(u_max_,
                                               b_max_,
                                               os_min_,
                                               N_eff_TX_max_,
                                               resampler_param_.L,
                                               resampler_param_.M,
                                               stf_bos_length_samples,
                                               stf_bos_rs_length_samples)) {
    dectnrp_assert(stf_bos_length_samples <= stf_bos_rs_length_samples,
                   "STF template before resampling longer than after");
    dectnrp_assert(stf_bos_rs_length_effective_samples <= stf_bos_rs_length_samples,
                   "effective STF template larger than entire STF");
}

stf_template_t::~stf_template_t() {
    for (auto& elem : stf_time_domain) {
        for (auto& elem_inner : elem) {
            free(elem_inner);
        }
        elem.clear();
    }

    stf_time_domain.clear();
}

const std::vector<cf_t*>& stf_template_t::get_stf_time_domain(const uint32_t b) const {
    return stf_time_domain[sp3::phyres::b2b_idx[b]];
}

common::vec2d<cf_t*> stf_template_t::generate_stf_time_domain(
    const uint32_t u_max,
    const uint32_t b_max,
    const uint32_t os_min,
    const uint32_t N_eff_TX_max,
    const uint32_t L,
    const uint32_t M,
    const uint32_t stf_bos_length_samples_compare,
    const uint32_t stf_bos_rs_length_samples_compare) {
    // local container for STF templates
    common::vec2d<cf_t*> ret;

    // what is the constant nominal system sample rate?
    const uint32_t samp_rate = u_max * b_max * constants::samp_rate_min_u_b * os_min;

    // how many subcarriers can we fit into that bandwidth?
    const uint32_t N_b_DFT_os = samp_rate / (u_max * constants::subcarrier_spacing_min_u_b);

    // for u=1, the cyclic prefix has a length of 3/4 IFFT-lengths, otherwise 5/4
    const uint32_t stf_bos_length_cp = (N_b_DFT_os / 4) * ((u_max == 1) ? 3 : 5);
    const uint32_t stf_bos_length = stf_bos_length_cp + N_b_DFT_os;
    const uint32_t stf_bos_rs_length = stf_bos_length * L / M;

    dectnrp_assert(stf_bos_length == stf_bos_length_samples_compare,
                   "STF template length incorrect");
    dectnrp_assert(stf_bos_rs_length == stf_bos_rs_length_samples_compare,
                   "STF template length incorrect");

    // load all relevant STFs in frequency domain
    sp3::stf_t stf(b_max, N_eff_TX_max, 1.0f);

    // for translation from frequency to time domain
    dft::ofdm_t ofdm;
    dft::get_ofdm(ofdm, N_b_DFT_os);

    // in time domain, we have to resample from DECTNRP sample rates to hw samples rates
    std::unique_ptr<resampler_t> resampler;
    resampler = std::make_unique<resampler_t>(
        1,
        L,
        M,
        resampler_param_t::f_pass_norm[resampler_param_t::user_t::TX][os_min],
        resampler_param_t::f_stop_norm[resampler_param_t::user_t::TX][os_min],
        resampler_param_t::PASSBAND_RIPPLE_DONT_CARE,
        resampler_param_t::f_stop_att_dB[resampler_param_t::user_t::TX][os_min]);

    // how many samples will the resampler put out with tail samples?
    const uint32_t N_sar = resampler->get_N_samples_after_resampling(stf_bos_length);

    dectnrp_assert(stf_bos_rs_length <= N_sar, "Length of STF incorrect");

    // local work buffers for both domains
    std::vector<cf_t> stf_fd(N_b_DFT_os);
    std::vector<cf_t> stf_td(stf_bos_length);
    std::vector<cf_t> stf_td_rs(N_sar);

    // init for each value of b ...
    for (uint32_t b_idx = 0; b_idx <= sp3::phyres::b2b_idx[b_max]; ++b_idx) {
        // new row
        ret.push_back(std::vector<cf_t*>());

        const uint32_t b = sp3::phyres::b_idx2b[b_idx];
        const uint32_t N_b_DFT = sp3::phyres::N_b_DFT_lut[b_idx];

        // offset includes guards and additional guards due to oversampling
        const uint32_t insert_offset =
            (N_b_DFT_os - N_b_DFT) / 2 + sp3::phyres::guards_bottom_lut[b_idx];

        // scaling factor to get average power of 1
        const float scale = 1.0f / std::sqrt(float(sp3::phyres::N_b_OCC_lut[b_idx] / 4));

        // ... and each value of N_eff_TX_max_ of radio device class
        for (uint32_t N_eff_TX = 1; N_eff_TX <= N_eff_TX_max; N_eff_TX *= 2) {
            // load reference to prefilled STF symbol in frequency domain (includes DC carrier)
            const std::vector<cf_t>& stf_fd_this = stf.get_stf(b, N_eff_TX);

            // zero and copy into vector at right position
            srsran_vec_cf_zero(stf_fd.data(), N_b_DFT_os);
            srsran_vec_cf_copy(&stf_fd[insert_offset], &stf_fd_this[0], stf_fd_this.size());

            // fftw uses mirrored spectrum
            dft::mem_mirror(stf_fd.data(), N_b_DFT_os);

            // IFFT, CP and scale
            dft::single_symbol_tx_ofdm_zero_copy(
                ofdm, stf_fd.data(), stf_td.data(), stf_bos_length_cp);
            srsran_vec_sc_prod_cfc_simd(stf_td.data(), scale, stf_td.data(), stf_bos_length);

            // to apply the STF cover sequence, we need to know the number of samples per STF
            // pattern
            const uint32_t N_samples_STF_pattern_os =
                constants::N_samples_stf_pattern * b * N_b_DFT_os / N_b_DFT;

            // apply STF cover sequence
            sp3::stf_t::apply_cover_sequence(
                stf_td.data(), stf_td.data(), u_max, N_samples_STF_pattern_os);

            resampler->reset();

            // resampler requires references to vectors as input
            std::vector<const cf_t*> inp{stf_td.data()};
            std::vector<cf_t*> out{stf_td_rs.data()};

            uint32_t N_sar_cnt = resampler->resample(inp, out, stf_bos_length);

            // resample remaining samples in history
            std::vector<cf_t*> out_final{&stf_td_rs[N_sar_cnt]};
            N_sar_cnt += resampler->resample_final_samples(out_final);

            dectnrp_assert(N_sar == N_sar_cnt, "Nof resampled STF samples incorrect");

            // allocate new memory and copy truncated STF
            cf_t* tmp = srsran_vec_cf_malloc(stf_bos_rs_length);
            srsran_vec_cf_copy(tmp, stf_td_rs.data(), stf_bos_rs_length);

            // add to return value, must be later freed externally
            ret[b_idx].push_back(tmp);
        }
    }

    dft::free_ofdm(ofdm);

    return ret;
}

}  // namespace dectnrp::phy
