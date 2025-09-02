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

#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "dectnrp/common/multidim.hpp"
#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_statistics.hpp"
#include "dectnrp/phy/rx/rx_synced/channel_estimation/wiener.hpp"
#include "dectnrp/phy/rx/rx_synced/processing_stage.hpp"
#include "dectnrp/phy/rx/rx_synced/rx_synced_param.hpp"

namespace dectnrp::phy {

class channel_lut_t {
    public:
        explicit channel_lut_t(const uint32_t b_max_,
                               const uint32_t N_eff_TX_max_,
                               const channel_statistics_t channel_statistics_);
        ~channel_lut_t();

        channel_lut_t() = delete;
        channel_lut_t(const channel_lut_t&) = delete;
        channel_lut_t& operator=(const channel_lut_t&) = delete;
        channel_lut_t(channel_lut_t&&) = delete;
        channel_lut_t& operator=(channel_lut_t&&) = delete;

        /// configuration of packet, set once
        void set_configuration_packet(const uint32_t b_idx_, const uint32_t N_eff_TX_);

        /// configuration of processing stage (ps), set for each processing stage
        void set_configuration_ps(const bool chestim_mode_lr, const uint32_t ps_idx_);

        /**
         * \brief Assuming all DRS channel estimates of a processing stage arranged as a vector: At
         * what offset in that vector does optimal weighting begin for each subcarrier at the given
         * OFDM symbol index? One vector element per transmit stream 0 to 4 or 3 to 7.
         *
         * \param ofdm_symb_ps_idx OFDM symbol index relative to processing stage
         * \return
         */
        const std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*>& get_idx_pilot_symb(
            const uint32_t ofdm_symb_ps_idx);

        /**
         * \brief Index of the optimal weight vector for each subcarrier of the given OFDM symbol.
         * One vector element per transmit stream 0 to 4 or 3 to 7.
         *
         * \param ofdm_symb_ps_idx OFDM symbol index relative to processing stage
         * \return
         */
        const std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*>& get_idx_weights_symb(
            const uint32_t ofdm_symb_ps_idx);

        /// one large vector with all weight vectors required, across all beta and processing stage
        /// configurations
        const RX_SYNCED_PARAM_WEIGHTS_TYPE* get_weight_vecs() const;

        /// number of DRS channel estimates to use for interpolation/extrapolation/smoothing
        uint32_t get_nof_drs_subc_interp() const { return nof_drs_subc_interp; }

        const uint32_t b_max;
        const uint32_t N_eff_TX_max;
        const channel_statistics_t channel_statistics;

    private:
        // ##################################################
        // variables depending of packet and processing stage

        /// configuration of current packet set in set_configuration_packet()
        uint32_t b_idx;
        uint32_t N_eff_TX;

        /// configuration of current processing stage set in set_configuration_ps()
        uint32_t ps_idx;
        processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>* idx_pilot_effective;
        processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>* idx_weights_effective;
        RX_SYNCED_PARAM_WEIGHTS_TYPE* lut_x_weight_vecs_effective;
        uint32_t nof_drs_subc_interp;

        /// preallocated vectors returned as reference, avoids allocating many small vectors
        std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*> idx_pilot_symb;
        std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*> idx_weights_symb;

        /// small helper for ts_idx switching
        static void swap_upper_lower_half(std::vector<RX_SYNCED_PARAM_LUT_IDX_TYPE*>& in);

        // ##################################################
        // variables instantiated once in the constructor

        class lut_t {
            public:
                /**
                 * \brief A single lut_t is defined by one value of
                 * 1. N_b_OCC_plus_DC = 56*b+1
                 * 2. ps_t_length = N_step_virtual + 1
                 * 3. N_eff_TX_max.
                 *
                 * Possible values of N_b_OCC_plus_DC = 57,113,225,449,673,897.
                 * Possible values of ps_t_length     = 1,6,11.
                 * Possible values of N_eff_TX_max    = 4.
                 *
                 * A single lut_t contains the optimal pilot indices, and the corresponding weight
                 * vector indices.
                 *
                 * \param N_b_OCC_plus_DC
                 * \param ps_t_length
                 * \param N_eff_TX_max
                 */
                explicit lut_t(const uint32_t N_b_OCC_plus_DC,
                               const uint32_t ps_t_length,
                               const uint32_t N_eff_TX_max) {
                    idx_pilot = std::make_unique<processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>>(
                        N_b_OCC_plus_DC, ps_t_length, N_eff_TX_max);

                    idx_weights =
                        std::make_unique<processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>>(
                            N_b_OCC_plus_DC, ps_t_length, N_eff_TX_max);
                };

                std::unique_ptr<processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>> idx_pilot;
                std::unique_ptr<processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>> idx_weights;
        };

        /**
         * \brief We differentiate three vectors of lut_t:
         *
         * 1. lut_0_vec: Estimate channel only at the first DRS symbol in a processing stage and use
         *               that channel estimate for the rest of the processing stage.
         * 2. lut_5_vec: Estimate channel between two OFDM symbols with DRS cells, N_step=5.
         * 3. lut_10_vec: Estimate channel between two OFDM symbols with DRS cells, N_step=10.
         *
         * Each vector lut_x_vec contains one lut_t per value of b, i.e. the maximum vector length
         * is six since b={1,2,4,8,12,16}.
         *
         * Each vector lut_x_vec also has a corresponding pointer lut_x_weight_vecs which contains
         * weight vectors for interpolation, extrapolation and smoothing. Weight vectors are always
         * calculated only for the maximum value of b of a radio device class. The weight vectors
         * for smaller values of b are a subset.
         *
         * Each lut_t, i.e. one of the vector elements, is always instantiated for 4 transmit
         * streams with index 0,1,2,3.
         */
        std::vector<lut_t> lut_0_vec;
        std::vector<lut_t> lut_5_vec;
        std::vector<lut_t> lut_10_vec;
        RX_SYNCED_PARAM_WEIGHTS_TYPE* lut_0_weight_vecs = nullptr;
        RX_SYNCED_PARAM_WEIGHTS_TYPE* lut_5_weight_vecs = nullptr;
        RX_SYNCED_PARAM_WEIGHTS_TYPE* lut_10_weight_vecs = nullptr;

        // ##################################################
        // helper functions and variables for instantiation of lut_x_vec and lut_x_weight_vecs

        /// helper to initialize one lut_x_vec + corresponding lut_x_weight_vecs
        static void init_lut_x_vec(const uint32_t N_step_virtual,
                                   const uint32_t b_max,
                                   const channel_statistics_t& channel_statistics,
                                   std::vector<lut_t>& lut_x_vec,
                                   RX_SYNCED_PARAM_WEIGHTS_TYPE*& lut_x_weight_vecs);

        /// helper to initialize one lut_t entry of lut_x_vec
        static void init_lut(const uint32_t N_step_virtual,
                             const uint32_t b,
                             const channel_statistics_t& channel_statistics,
                             processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>& idx_pilot,
                             processing_stage_t<RX_SYNCED_PARAM_LUT_IDX_TYPE>& idx_weights,
                             common::vec2d<RX_SYNCED_PARAM_WEIGHTS_TYPE>& weight_vecs_tmp);

        // ##################################################
        // helper types and functions used in init_lut()

        /// To measure distances between data points and DRS pilots, we need a coordinate system for
        /// the subcarriers. Here, f is frequency and t is time.
        struct coord_t {
                coord_t(const double f_, const double t_)
                    : f(f_),
                      t(t_) {};
                const double f, t;

                static double get_dist(const coord_t A, const coord_t B) {
                    return std::sqrt(std::pow(A.f - B.f, 2.0) + std::pow(A.t - B.t, 2.0));
                }
                static double get_df(const coord_t src, const coord_t dst) { return dst.f - src.f; }
                static double get_dt(const coord_t src, const coord_t dst) { return dst.t - src.t; }
        };

        /// Step 0: create vector of consecutive DRS coordinates
        static std::vector<coord_t> s0_calc_drs_pilot_vec(const common::vec2d<uint32_t>& k_i_l,
                                                          const common::vec2d<uint32_t>& k_i_r,
                                                          const uint32_t ts_idx,
                                                          const uint32_t N_step_virtual);

        /// Step 1: find optimal index in vector of consecutive DRS pilots
        static RX_SYNCED_PARAM_LUT_IDX_TYPE s1_find_opt_idx_pilot(
            const double f,
            const double t,
            const std::vector<coord_t>& coord_drs_vec,
            const uint32_t nof_drs_subc_interp,
            /// for speed optimization
            const uint32_t opt_idx_pilot_prev);

        /// Step 2: calculate optimal weight vector for given subcarrier and consecutive DRS pilots
        static std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE> s2_calc_weight_vec(
            const double f,
            const double t,
            const std::vector<coord_t>& coord_drs_vec,
            const uint32_t nof_drs_subc_interp,
            const channel_statistics_t& channel_statistics,
            const uint32_t opt_idx_pilot,
            /// for speed optimization
            const uint32_t opt_idx_pilot_prev,
            wiener_t<RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL>& wiener);

        /// Step 2 helper: calculate correlation between two (f,t)-coordinates
        static RX_SYNCED_PARAM_WEIGHTS_TYPE_INTERNAL s2_calc_correlation_noisefree(
            const coord_t& A, const coord_t& B, const channel_statistics_t& channel_statistics);

        /// Step 3: find equal weight vector and if unknown attach to weight_vecs_tmp, return index
        static RX_SYNCED_PARAM_LUT_IDX_TYPE s3_find_weight_vec_index(
            const std::vector<RX_SYNCED_PARAM_WEIGHTS_TYPE>& weight_vec,
            common::vec2d<RX_SYNCED_PARAM_WEIGHTS_TYPE>& weight_vecs_tmp);
};

}  // namespace dectnrp::phy
