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
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include "srsran/config.h"
}

namespace dectnrp::simulation {

class link_t {
    public:
        explicit link_t(const uint32_t samp_rate_,
                        const uint32_t spp_size_,
                        const uint32_t pdp_idx_,
                        const float tau_rms_ns_,
                        const float fD_Hz_);
        ~link_t();

        /// minimize number of calculated taps
        void set_pdp();

        /// print current channel configuration
        void print_pdp(const std::string prefix) const;

        /// randomize sum of sinusoids settings
        void randomize();

        /**
         * \brief We assume the channel is reciprocal between simulators. Thus, if both simulator
         * input the same inp from their respective ends, they will receive the same out if called
         * at the same point in time time_64. Calls must have consecutive time_64, as the class
         * internally keeps a history of former samples.
         *
         * \param inp
         * \param out
         * \param primary_direction
         * \param time_64
         */
        void pass_through_link(const cf_t* inp,
                               cf_t* out,
                               const bool primary_direction,
                               const int64_t time_64);

        const uint32_t samp_rate;
        const uint32_t spp_size;
        const uint32_t pdp_idx;
        const float tau_rms_ns;
        const float fD_Hz;

    private:
        // we have two directions, each has its own history
        std::array<cf_t*, 2> history_stage_arr;
        cf_t* sinusoid_stage;
        cf_t* superposition_stage;

        // ##################################################
        // Doppler and delay spread

        static constexpr float tau_rms_ns_max = 2000;
        static constexpr float fD_Hz_max = 2000;

        static constexpr uint32_t WIRELESS_CHANNEL_DOUBLY_NOF_PROFILE = 3;
        static constexpr uint32_t WIRELESS_CHANNEL_DOUBLY_NOF_TAPS = 9;

        /* Measured tau_rms values for the profiles given below:
         *  0:  43.13 ns
         *  1: 356.65 ns
         *  2: 990.93 ns
         */

        /// we use generic power delay profiles and ...
        /// https://github.com/srsran/srsRAN_4G/blob/master/lib/src/phy/channel/fading.c
        static constexpr double
            tap_delays_ns_generic[WIRELESS_CHANNEL_DOUBLY_NOF_PROFILE]
                                 [WIRELESS_CHANNEL_DOUBLY_NOF_TAPS] = {
                                     {0, 30, 70, 90, 110, 190, 410, NAN, NAN},
                                     {0, 30, 150, 310, 370, 710, 1090, 1730, 2510},
                                     {0, 50, 120, 200, 230, 500, 1600, 2300, 5000}};

        /// https://github.com/srsran/srsRAN_4G/blob/master/lib/src/phy/channel/fading.c
        static constexpr double tap_powers_dB_generic
            [WIRELESS_CHANNEL_DOUBLY_NOF_PROFILE][WIRELESS_CHANNEL_DOUBLY_NOF_TAPS] = {
                {+0.0f, -1.0f, -2.0f, -3.0f, -8.0f, -17.2f, -20.8f, NAN, NAN},
                {+0.0f, -1.5f, -1.4f, -3.6f, -0.6f, -9.1f, -7.0f, -12.0f, -16.9f},
                {-1.0f, -1.0f, -1.0f, +0.0f, +0.0f, +0.0f, -3.0f, -5.0f, -7.0f}};

        /// ... put the required taps after scaling into these vector
        std::vector<uint32_t> tap_delays_smpl;
        std::vector<double> tap_powers_linear;

        /// root of the second central moment of the normalized delay power density spectrum
        static double get_tau_rms_ns(const double* tap_delays,
                                     const double* tap_powers_dB,
                                     uint32_t nof_val);

        // ##################################################
        // Sum of sinusoids

        /// Doppler frequencies within deadband are set to zero
        static constexpr double DOPPLER_FD_HZ_DEADBAND = 0.01f;

        /// Matlab uses 40 sinusoids
        static constexpr uint32_t WIRELESS_CHANNEL_DOUBLY_NOF_SINUSOIDS = 40;

        /// we save the period of each frequency in samples at samp_rate
        int64_t period_smpl[WIRELESS_CHANNEL_DOUBLY_NOF_TAPS]
                           [WIRELESS_CHANNEL_DOUBLY_NOF_SINUSOIDS];

        /// random phase real and imaginary for each sinusoid
        float phase_inital_rad[WIRELESS_CHANNEL_DOUBLY_NOF_TAPS]
                              [WIRELESS_CHANNEL_DOUBLY_NOF_SINUSOIDS];
};

}  // namespace dectnrp::simulation
