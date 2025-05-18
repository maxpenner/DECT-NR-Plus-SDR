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

#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_lut.hpp"

#include <cstring>
#include <limits>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part3/drs.hpp"
#include "dectnrp/sections_part3/physical_resources.hpp"

static constexpr uint32_t N_EFF_TX_MAX_PRECALC = 4U;

// for equal weight vectors, we compare real and imaginary part
static constexpr double PHY_RX_RX_SYNCED_CHANNEL_ESTIMATION_EQUALITY_THRESHOLD = 1e-4;

namespace dectnrp::phy {

channel_lut_t::channel_lut_t(const uint32_t b_max_,
                             const uint32_t N_eff_TX_max_,
                             const channel_statistics_t channel_statistics_)
    : b_max(b_max_),
      N_eff_TX_max(N_eff_TX_max_),
      channel_statistics(channel_statistics_) {
    // preallocate
    idx_pilot_symb.resize(N_EFF_TX_MAX_PRECALC);
    idx_weights_symb.resize(N_EFF_TX_MAX_PRECALC);

    init_lut_x_vec(0, b_max, channel_statistics, lut_0_vec, lut_0_weight_vecs);
    init_lut_x_vec(5, b_max, channel_statistics, lut_5_vec, lut_5_weight_vecs);

    // initialize only if necessary
    if (N_eff_TX_max >= 4) {
        init_lut_x_vec(10, b_max, channel_statistics, lut_10_vec, lut_10_weight_vecs);
    }
}

channel_lut_t::~channel_lut_t() {
    free(lut_0_weight_vecs);
    free(lut_5_weight_vecs);
    free(lut_10_weight_vecs);
}

void channel_lut_t::set_configuration_packet(const uint32_t b_idx_, const uint32_t N_eff_TX_) {
    b_idx = b_idx_;
    N_eff_TX = N_eff_TX_;
}

void channel_lut_t::set_configuration_ps(const bool chestim_mode_lr, const uint32_t ps_idx_) {
    ps_idx = ps_idx_;

    if (chestim_mode_lr) {
        if (N_eff_TX <= 2) {
            // lut_5_vec
            idx_pilot_effective = lut_5_vec[b_idx].idx_pilot.get();
            idx_weights_effective = lut_5_vec[b_idx].idx_weights.get();
            lut_x_weight_vecs_effective = lut_5_weight_vecs;
        } else {
            // lut_10_vec
            idx_pilot_effective = lut_10_vec[b_idx].idx_pilot.get();
            idx_weights_effective = lut_10_vec[b_idx].idx_weights.get();
            lut_x_weight_vecs_effective = lut_10_weight_vecs;
        }
        nof_drs_subc_interp = channel_statistics.nof_drs_interp_lr;
    } else {
        // lut_0_vec
        idx_pilot_effective = lut_0_vec[b_idx].idx_pilot.get();
        idx_weights_effective = lut_0_vec[b_idx].idx_weights.get();
        lut_x_weight_vecs_effective = lut_0_weight_vecs;
        nof_drs_subc_interp = channel_statistics.nof_drs_interp_l;
    }
}

const std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*>& channel_lut_t::get_idx_pilot_symb(
    const uint32_t ofdm_symb_ps_idx) {
    // load in default order
    idx_pilot_effective->get_stage_prealloc(ofdm_symb_ps_idx, idx_pilot_symb);

    /* Figure 4.5-2 and 4.5-3
     *
     * What is the transmit stream index order for an EVEN processing stage index?
     * OFDM symbol index 0:                        0 1 2 3
     * OFDM symbol index 1:  4 5 6 7 equivalent to 0 1 2 3
     * OFDM symbol index 5:                        2 3 0 1
     * OFDM symbol index 10:                       2 3 0 1
     * OFDM symbol index 11: 6 7 4 5 equivalent to 2 3 0 1
     *
     * What is the transmit stream index order for an ODD processing stage index?
     * OFDM symbol index 0:                        2 3 0 1
     * OFDM symbol index 1:  6 7 4 5 equivalent to 2 3 0 1
     * OFDM symbol index 5:                        0 1 2 3
     * OFDM symbol index 10:                       0 1 2 3
     * OFDM symbol index 11: 4 5 6 7 equivalent to 0 1 2 3
     */
    if (ps_idx % 2 == 1) {
        swap_upper_lower_half(idx_pilot_symb);
    }

    return idx_pilot_symb;
}

const std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*>& channel_lut_t::get_idx_weights_symb(
    const uint32_t ofdm_symb_ps_idx) {
    // load in default order
    idx_weights_effective->get_stage_prealloc(ofdm_symb_ps_idx, idx_weights_symb);

    /* Figure 4.5-2 and 4.5-3
     *
     * What is the transmit stream index order for an EVEN processing stage index?
     * OFDM symbol index 0:                        0 1 2 3
     * OFDM symbol index 1:  4 5 6 7 equivalent to 0 1 2 3
     * OFDM symbol index 5:                        2 3 0 1
     * OFDM symbol index 10:                       2 3 0 1
     * OFDM symbol index 11: 6 7 4 5 equivalent to 2 3 0 1
     *
     * What is the transmit stream index order for an ODD processing stage index?
     * OFDM symbol index 0:                        2 3 0 1
     * OFDM symbol index 1:  6 7 4 5 equivalent to 2 3 0 1
     * OFDM symbol index 5:                        0 1 2 3
     * OFDM symbol index 10:                       0 1 2 3
     * OFDM symbol index 11: 4 5 6 7 equivalent to 0 1 2 3
     */
    if (ps_idx % 2 == 1) {
        swap_upper_lower_half(idx_weights_symb);
    }

    return idx_weights_symb;
}

const RX_SYNCED_PARAM_WEIGHTS_TYPE* channel_lut_t::get_weight_vecs() const {
    return lut_x_weight_vecs_effective;
}

void channel_lut_t::swap_upper_lower_half(std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*>& in) {
    dectnrp_assert(in.size() == 4, "incorrect length, should be 4");

    RX_SYNCED_PARAM_LUT_IDX_TYPE* A = in[0];
    RX_SYNCED_PARAM_LUT_IDX_TYPE* B = in[1];

    in[0] = in[2];
    in[1] = in[3];
    in[2] = A;
    in[3] = B;
}

void channel_lut_t::init_lut_x_vec(const uint32_t N_step_virtual,
                                   const uint32_t b_max,
                                   const channel_statistics_t& channel_statistics,
                                   std::vector<lut_t>& lut_x_vec,
                                   RX_SYNCED_PARAM_WEIGHTS_TYPE*& lut_x_weight_vecs) {
    // How long is the processing stage in time domain?
    const uint32_t ps_t_length = N_step_virtual + 1;

    // convert to maximum index of b
    const uint32_t b_idx_max = section3::phyres::b2b_idx[b_max];

    // first init sizes for every value of b
    for (uint32_t b_idx = 0; b_idx <= b_idx_max; ++b_idx) {
        lut_x_vec.push_back(
            lut_t(section3::phyres::N_b_OCC_plus_DC_lut[b_idx], ps_t_length, N_EFF_TX_MAX_PRECALC));
    }

    /* When we create every weight vector for the maximum value of b, there is no need for new
     * weight vectors for smaller values of b. Thus, create one temporary C++ vector of weight
     * vectors, which will be converted later. We iterate over each b, starting with b_max.
     */
    common::vec2d<RX_SYNCED_PARAM_WEIGHTS_TYPE> weight_vecs_tmp;

    // fill each lut_t, but in reverse order starting with b_max
    for (int32_t b_idx = b_idx_max; b_idx >= 0; b_idx--) {
        // remember size to check that only for b_max new values are added
        const uint32_t weight_vecs_tmp_size_before = weight_vecs_tmp.size();

        // current dimensions
        const uint32_t b = section3::phyres::b_idx2b[b_idx];

        // fill one element of init_lut_x_vec
        init_lut(N_step_virtual,
                 b,
                 channel_statistics,
                 *(lut_x_vec[b_idx].idx_pilot.get()),
                 *(lut_x_vec[b_idx].idx_weights.get()),
                 weight_vecs_tmp);

        if (static_cast<uint32_t>(b_idx) < b_idx_max) {
            dectnrp_assert(weight_vecs_tmp.size() == weight_vecs_tmp_size_before,
                           "smaller b has added new weight vectors");
        }
    }

    // finally, we have to transform weight_vecs_tmp into one large memory block

    // all vectors have the same number of elements
    const uint32_t nof_drs_subc_interp = weight_vecs_tmp[0].size();

    // allocate memory
#if RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL
    lut_x_weight_vecs = srsran_vec_f_malloc(weight_vecs_tmp.size() * nof_drs_subc_interp);
#elif RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP
    lut_x_weight_vecs = srsran_vec_cf_malloc(weight_vecs_tmp.size() * nof_drs_subc_interp);
#endif

    // copy into memory block
    for (uint32_t i = 0; i < weight_vecs_tmp.size(); ++i) {
        memcpy(&lut_x_weight_vecs[i * nof_drs_subc_interp],
               &weight_vecs_tmp[i][0],
               nof_drs_subc_interp * sizeof(RX_SYNCED_PARAM_WEIGHTS_TYPE));
    }
}

void channel_lut_t::init_lut(const uint32_t N_step_virtual,
                             const uint32_t b,
                             const channel_statistics_t& channel_statistics,
                             processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>& idx_pilot,
                             processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>& idx_weights,
                             common::vec2d<RX_SYNCED_PARAM_WEIGHTS_TYPE>& weight_vecs_tmp) {
    // get dimensions of processing stages
    const uint32_t ps_t_length = idx_pilot.N_t_domain_max;
    const uint32_t N_b_OCC_plus_DC = idx_pilot.N_f_domain_max;

    // set processing stage configuration for current value of b
    idx_pilot.set_configuration(N_b_OCC_plus_DC, ps_t_length);
    idx_weights.set_configuration(N_b_OCC_plus_DC, ps_t_length);

    // generate DRS symbols for all values of b and all TS
    section3::drs_t drs(b, N_EFF_TX_MAX_PRECALC);

    // set current DRS configuration
    drs.set_configuration(b, N_EFF_TX_MAX_PRECALC);

    // load indices of left DRS symbol, see Figure 4.5-2 and Figure 4.5-3
    const common::vec2d<uint32_t>& k_i_l = drs.get_k_i();

    // jump to next DRS symbol
    drs.get_ts_idx_last_and_advance();

    // load indices of right DRS symbol, see Figure 4.5-2 and Figure 4.5-3
    const common::vec2d<uint32_t>& k_i_r = drs.get_k_i();

    // how many consecutive DRS values are used for interpolation?
    const uint32_t nof_drs_subc_interp = (N_step_virtual > 0) ? channel_statistics.nof_drs_interp_lr
                                                              : channel_statistics.nof_drs_interp_l;

    // go over each transmit stream
    for (uint32_t ts_idx = 0; ts_idx < N_EFF_TX_MAX_PRECALC; ++ts_idx) {
        /* At this point, we have a specific value of b, and N_EFF_TX_MAX_PRECALC a.k.a the number
         * of transmit streams. In the current for-loop, we have entered one of those transmit
         * streams with index ts_idx.
         *
         * Our goal now is to load the DRS symbols coordinates for this combination in the same
         * form as they will be later provided in channel_antenna.cpp, i.e. as a vector of
         * consecutive, possibly interlaced, zero-forced DRS cell channel estimates.
         */
        std::vector<coord_t> coord_drs_vec =
            s0_calc_drs_pilot_vec(k_i_l, k_i_r, ts_idx, N_step_virtual);

        dectnrp_assert(!(N_step_virtual > 0 &&
                         coord_drs_vec.size() != 2 * section3::drs_t::get_nof_drs_subc(b)),
                       "Incorrect number of DRS symbols.");
        dectnrp_assert(
            !(N_step_virtual == 0 && coord_drs_vec.size() != section3::drs_t::get_nof_drs_subc(b)),
            "Incorrect number of DRS symbols.");

        /* We now have inserted the coordinates of DRS symbol consecutively into a vector. Next
         * we have to find the optimal index for each subcarrier in the processing stage which
         * gives the minimum average distance to the specified number of consecutive pilots in
         * coord_drs_vec. We start at OFDM symbol index t=1, see Figure 4.5-2 and Figure 4.5-3.
         *
         * At that optimal index, we then determine the optimal interpolations weights. These
         * weights are appended to weight_vecs_tmp if unknown yet, and the index within that vector
         * is saved.
         *
         * Iterate over each OFDM symbol that requires channel estimation ...
         */
        for (uint32_t t = 1; t <= ps_t_length; ++t) {
            /* Get access to specific OFDM symbol in processing stage. Note that while in
             * Figure 4.5-2 and Figure 4.5-3 the first OFDM symbol index with DRS pilots is t=1,
             * we start filling the processing stage at symbol index 0.
             */
            RX_SYNCED_PARAM_LUT_IDX_TYPE* wp_idx_pilot =
                idx_pilot.get_stage_specific(t - 1, ts_idx);
            RX_SYNCED_PARAM_LUT_IDX_TYPE* wp_idx_weights =
                idx_weights.get_stage_specific(t - 1, ts_idx);

            /* Within an OFDM symbol, the optimal index is monotonically increasing. So for any
             * subcarrier, we can efficiently start our search at the optimal index of the last
             * subcarrier.
             */
            uint32_t opt_idx_pilot_prev = 0;

            /* If two subcarrier have the same optimal index for the pilots, they use the exact same
             * pilots and we can reuse Rpp and Rppinv of the wiener_t class, i.e. we don't have to
             * recalculate an inverse matrix for every subcarrier.
             */
            wiener_t<RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL> wiener(nof_drs_subc_interp);

            // ... and each of its subcarrier
            for (uint32_t f = 0; f < N_b_OCC_plus_DC; ++f) {
                // find optimal index and directly write into respective processing stage
                wp_idx_pilot[f] = s1_find_opt_idx_pilot(static_cast<double>(f),
                                                        static_cast<double>(t),
                                                        coord_drs_vec,
                                                        nof_drs_subc_interp,
                                                        // for speed optimization
                                                        opt_idx_pilot_prev);

                // create optimal interpolation weight vec, internally solves Wiener-Hopf equation
                std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE> weight_vec =
                    s2_calc_weight_vec(static_cast<double>(f),
                                       static_cast<double>(t),
                                       coord_drs_vec,
                                       nof_drs_subc_interp,
                                       channel_statistics,
                                       wp_idx_pilot[f],
                                       // for speed optimization
                                       opt_idx_pilot_prev,
                                       wiener);

                // Check if weight vector is already known. Is so, return index. Otherwise append
                // and return index of last entry.
                wp_idx_weights[f] = s3_find_weight_vec_index(weight_vec, weight_vecs_tmp);

#ifdef RX_SYNCED_PARAM_CHANNEL_LUT_OPT_INDEX_PREVIOUS
                opt_idx_pilot_prev = wp_idx_pilot[f];
#endif
            }
        }
    }
}

std::vector<channel_lut_t::coord_t> channel_lut_t::s0_calc_drs_pilot_vec(
    const common::vec2d<uint32_t>& k_i_l,
    const common::vec2d<uint32_t>& k_i_r,
    const uint32_t ts_idx,
    const uint32_t N_step_virtual) {
    // return value
    std::vector<coord_t> coord_drs_vec;

    // indices of left and right DRS symbol, see Figure 4.5-2 and Figure 4.5-3
    const double symb_idx_0 = 1.0;
    const double symb_idx_1 = symb_idx_0 + static_cast<double>(N_step_virtual);

    // four different cases have to be distinguished
    if (N_step_virtual > 0) {
        // see Figure 4.5-3
        if (ts_idx <= 1) {
            // interlace
            for (uint32_t p_idx = 0; p_idx < k_i_l[ts_idx].size(); ++p_idx) {
                coord_drs_vec.push_back(coord_t(k_i_l[ts_idx][p_idx], symb_idx_0));
                coord_drs_vec.push_back(coord_t(k_i_r[ts_idx][p_idx], symb_idx_1));
            }
        } else {
            // interlace
            for (uint32_t p_idx = 0; p_idx < k_i_l[ts_idx].size(); ++p_idx) {
                coord_drs_vec.push_back(coord_t(k_i_r[ts_idx][p_idx], symb_idx_1));
                coord_drs_vec.push_back(coord_t(k_i_l[ts_idx][p_idx], symb_idx_0));
            }
        }
    } else {
        for (uint32_t p_idx = 0; p_idx < k_i_l[ts_idx].size(); ++p_idx) {
            coord_drs_vec.push_back(coord_t(k_i_l[ts_idx][p_idx], symb_idx_0));
        }
    }

    // assert
    for (uint32_t i = 1; i < coord_drs_vec.size(); ++i) {
        dectnrp_assert(coord_drs_vec[i - 1].f < coord_drs_vec[i].f,
                       "Frequencies must be strictly increasing.");
    }

    return coord_drs_vec;
}

RX_SYNCED_PARAM_LUT_IDX_TYPE channel_lut_t::s1_find_opt_idx_pilot(
    const double f,
    const double t,
    const std::vector<coord_t>& coord_drs_vec,
    const uint32_t nof_drs_subc_interp,
    // for speed optimization
    const uint32_t opt_idx_pilot_prev) {
    // set to some arbitrary large value
    double min_sum_distance = 1e9;

    // the return index
    uint32_t opt_idx_pilot = 0;

    // start at the bottom of Figure 4.5-1 and Figure 4.5-2 or at the last optimal index ...
    for (uint32_t i = opt_idx_pilot_prev; i <= coord_drs_vec.size() - nof_drs_subc_interp; ++i) {
        // ... and sum up Euclidian distances over current window of DRS pilots
        double min_sum_distance_this = 0;
        for (uint32_t j = i; j < i + nof_drs_subc_interp; ++j) {
            min_sum_distance_this += coord_t::get_dist(coord_t(f, t), coord_drs_vec[j]);
        }

        // if this sum is smaller, save new value
        if (min_sum_distance_this < min_sum_distance) {
            min_sum_distance = min_sum_distance_this;
            opt_idx_pilot = i;
        }

#ifdef RX_SYNCED_PARAM_CHANNEL_LUT_SEARCH_ABORT_THRESHOLD
        // abort search if sum distance is getting larger and crosses a threshold
        if (min_sum_distance_this >
            min_sum_distance * RX_SYNCED_PARAM_CHANNEL_LUT_SEARCH_ABORT_THRESHOLD) {
            break;
        }
#endif
    }

    dectnrp_assert(opt_idx_pilot <= std::numeric_limits<RX_SYNCED_PARAM_LUT_IDX_TYPE>::max(),
                   "Index too large.");

    return static_cast<RX_SYNCED_PARAM_LUT_IDX_TYPE>(opt_idx_pilot);
}

std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE> channel_lut_t::s2_calc_weight_vec(
    const double f,
    const double t,
    const std::vector<coord_t>& coord_drs_vec,
    const uint32_t nof_drs_subc_interp,
    const channel_statistics_t& channel_statistics,
    const uint32_t opt_idx_pilot,
    // for speed optimization
    const uint32_t opt_idx_pilot_prev,
    wiener_t<RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL>& wiener) {
    /* Always refresh Rpp and Rppinv if we are at first subcarrier (f==0.0). If we are, however, at
     * f>0.0 and the previous subcarrier had the same optimal index, omit the refresh of Rpp and
     * Rppinv.
     */
    if (f == 0.0 || opt_idx_pilot_prev != opt_idx_pilot) {
        // we have to recalculate Rpp, so go over all pilots and create matrix Rpp
        for (uint32_t row_idx = 0; row_idx < nof_drs_subc_interp; ++row_idx) {
            // position of this pilot
            const coord_t& A = coord_drs_vec[opt_idx_pilot + row_idx];

            for (uint32_t col_idx = 0; col_idx < nof_drs_subc_interp; ++col_idx) {
                // position of compare pilot
                const coord_t& B = coord_drs_vec[opt_idx_pilot + col_idx];

                // get noisefree correlation value
                const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL corr_tf =
                    s2_calc_correlation_noisefree(A, B, channel_statistics);

                // are we on the main diagonal?
                if (row_idx == col_idx) {
#if RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL
                    // if so, add real noise
                    const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL sigma = channel_statistics.sigma;
#elif RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP
                    // if so, add complex noise
                    const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL sigma{channel_statistics.sigma,
                                                                      0.0};
#endif
                    // on main diagonal
                    wiener.set_entry_Rpp(row_idx, col_idx, corr_tf + sigma);
                } else {
                    // off main diagonal
                    wiener.set_entry_Rpp(row_idx, col_idx, corr_tf);
                }
            }
        }

        // recalculate the inverse
        wiener.set_Rppinv();
    }

    // recalculate rdp, A holds coordinates of data
    const coord_t A = coord_t(f, t);
    for (uint32_t row_idx = 0; row_idx < nof_drs_subc_interp; ++row_idx) {
        // position of this pilot
        const coord_t& B = coord_drs_vec[opt_idx_pilot + row_idx];

        // get noisefree correlation value ...
        const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL corr_tf =
            s2_calc_correlation_noisefree(A, B, channel_statistics);

        // ... and set in vector
        wiener.set_entry_rdp(row_idx, corr_tf);
    }

    // determine optimal weights, true for normalization
    const std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL> w = wiener.get_Rppinv_x_rdp(true);

    // w is given in precise type, while w_ret is required in output type
    std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE> w_ret(nof_drs_subc_interp);

    // go over each entry, check validity and convert
    for (uint32_t row_idx = 0; row_idx < nof_drs_subc_interp; ++row_idx) {
#if RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL

        // convert from double/float to float
        const RX_SYNCED_PARAM_WEIGHTS_TYPE entry =
            static_cast<RX_SYNCED_PARAM_WEIGHTS_TYPE>(w[row_idx]);

        dectnrp_assert(std::isfinite(entry), "Entry of vector not finite");

#elif RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP

        // split into real and imag, convert from double/float to float
        const float entry_real = static_cast<float>(w[row_idx].real());
        const float entry_imag = static_cast<float>(w[row_idx].imag());

        dectnrp_assert(std::isfinite(entry_real), "Entry real of vector not finite.");
        dectnrp_assert(std::isfinite(entry_imag), "Entry imag of vector not finite.");

        const RX_SYNCED_PARAM_WEIGHTS_TYPE entry{entry_real, entry_imag};
#endif

        w_ret[row_idx] = entry;
    }

    return w_ret;
}

RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL channel_lut_t::s2_calc_correlation_noisefree(
    const coord_t& A, const coord_t& B, const channel_statistics_t& channel_statistics) {
    // spacing in time and frequency domain
    const double delta_f = coord_t::get_df(B, A);
    const double delta_t = coord_t::get_dt(B, A);

#if RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL

    // correlation in frequency domain, return type must be real
    const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL corr_f = channel_statistics_t::get_r_f_uni(
        channel_statistics.tau_rms_sec, delta_f * channel_statistics.delta_u_f);

    // correlation in time domain
    const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL corr_t = channel_statistics_t::get_r_t_jakes(
        channel_statistics.nu_max_hz, delta_t * channel_statistics.T_u_symb);

#elif RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP

    // correlation in frequency domain, return type must be complex
    const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL corr_f =
        channel_statistics_t::get_r_f_exp<RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL>(
            channel_statistics.tau_rms_sec, delta_f * channel_statistics.delta_u_f);

    // correlation in time domain
    const RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL corr_t{
        channel_statistics_t::get_r_t_jakes(channel_statistics.nu_max_hz,
                                            delta_t * channel_statistics.T_u_symb),
        0.0};

#endif

    return corr_f * corr_t;
}

RX_SYNCED_PARAM_LUT_IDX_TYPE channel_lut_t::s3_find_weight_vec_index(
    const std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE>& weight_vec,
    common::vec2d<RX_SYNCED_PARAM_WEIGHTS_TYPE>& weight_vecs_tmp) {
    // number of weights per weight vector
    const uint32_t nof_drs_subc_interp = weight_vec.size();

    // assume vector is unknown yet
    int32_t idx_of_known = -1;

    // we compare two vectors by subtracting them, sub is the result of that
    std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE> sub(nof_drs_subc_interp);

    // go over each already known vector
    for (uint32_t i = 0; i < weight_vecs_tmp.size(); ++i) {
#if RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_REAL

        // subtract
        srsran_vec_sub_fff(&weight_vec[0], &weight_vecs_tmp[i][0], &sub[0], nof_drs_subc_interp);

        // find largest absolute difference
        const uint32_t idx_of_max = srsran_vec_max_abs_fi(&sub[0], nof_drs_subc_interp);
        const double diff = sub[idx_of_max];

        // check if values are essentially the same
        if (std::abs(diff) < PHY_RX_RX_SYNCED_CHANNEL_ESTIMATION_EQUALITY_THRESHOLD) {
            idx_of_known = static_cast<int32_t>(i);
            break;
        }

#elif RX_SYNCED_PARAM_WEIGHTS_TYPE_CHOICE == RX_SYNCED_PARAM_WEIGHTS_TYPE_COMP

        // subtract
        srsran_vec_sub_ccc(&weight_vec[0], &weight_vecs_tmp[i][0], &sub[0], nof_drs_subc_interp);

        // find largest absolute difference
        const uint32_t idx_of_max = srsran_vec_max_abs_ci(&sub[0], nof_drs_subc_interp);
        const double diff_real = __real__(sub[idx_of_max]);
        const double diff_imag = __imag__(sub[idx_of_max]);

        // check if values are essentially the same
        if (std::abs(diff_real) < PHY_RX_RX_SYNCED_CHANNEL_ESTIMATION_EQUALITY_THRESHOLD &&
            std::abs(diff_imag) < PHY_RX_RX_SYNCED_CHANNEL_ESTIMATION_EQUALITY_THRESHOLD) {
            idx_of_known = static_cast<int32_t>(i);
            break;
        }
#endif
    }

    // if unknown, add new vector
    if (idx_of_known == -1) {
        weight_vecs_tmp.push_back(weight_vec);
        idx_of_known = static_cast<int32_t>(weight_vecs_tmp.size() - 1);
    }

    dectnrp_assert(idx_of_known >= 0, "Index negative.");
    dectnrp_assert(static_cast<uint32_t>(idx_of_known) <=
                       std::numeric_limits<RX_SYNCED_PARAM_LUT_IDX_TYPE>::max(),
                   "Index too large.");

    // return index of correct weight vector
    return static_cast<RX_SYNCED_PARAM_LUT_IDX_TYPE>(idx_of_known);
}

}  // namespace dectnrp::phy
