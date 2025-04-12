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

#include "dectnrp/simulation/wireless/link.hpp"

#include <volk/volk.h>

#include <numbers>
#include <numeric>

extern "C" {
#include "srsran/phy/utils/vector.h"
}

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/common/randomgen.hpp"

namespace dectnrp::simulation {

link_t::link_t(const uint32_t samp_rate_,
               const uint32_t spp_size_,
               const uint32_t pdp_idx_,
               const float tau_rms_ns_,
               const float fD_Hz_)
    : samp_rate(samp_rate_),
      spp_size(spp_size_),
      pdp_idx(pdp_idx_),
      tau_rms_ns(tau_rms_ns_),
      fD_Hz(fD_Hz_) {
    dectnrp_assert(pdp_idx < WIRELESS_CHANNEL_DOUBLY_NOF_PROFILE, "pdp_idx out of bound.");
    dectnrp_assert(0.0f <= tau_rms_ns && tau_rms_ns <= tau_rms_ns_max, "tau_rms_ns out of bound.");
    dectnrp_assert(0.0f <= fD_Hz && fD_Hz <= fD_Hz_max, "fD_Hz out of bound.");

    for (auto& history_stage : history_stage_arr) {
        history_stage = srsran_vec_cf_malloc(spp_size * 2);
    }

    sinusoid_stage = srsran_vec_cf_malloc(spp_size);
    superposition_stage = srsran_vec_cf_malloc(spp_size);

    set_pdp();
    randomize();
}

link_t::~link_t() {
    for (auto history_stage : history_stage_arr) {
        free(history_stage);
    }

    free(sinusoid_stage);
    free(superposition_stage);
}

void link_t::set_pdp() {
    // vectors must be cleared in case we redefine
    tap_delays_smpl.clear();
    tap_powers_linear.clear();

    const double Ts = 1.0 / static_cast<double>(samp_rate);

    // go over entire PDP and count the finite values
    uint32_t nof_taps_finite = 0;
    for (uint32_t i = 0; i < WIRELESS_CHANNEL_DOUBLY_NOF_TAPS; ++i) {
        if (std::isfinite(tap_delays_ns_generic[pdp_idx][i])) {
            ++nof_taps_finite;
        } else {
            break;
        }
    }

    // What is the tau rms in nanoseconds of the generic power delay profile picked?
    const double tau_rms_ns_generic = get_tau_rms_ns(
        tap_delays_ns_generic[pdp_idx], tap_powers_dB_generic[pdp_idx], nof_taps_finite);

    // What is then the required time domain scaling factor?
    const double scale = tau_rms_ns / tau_rms_ns_generic;

    // go over each generic delay
    for (uint32_t i = 0; i < nof_taps_finite; ++i) {
        // convert delay to closest sample at sample rate
        const uint32_t A = static_cast<uint32_t>(
            std::floor(tap_delays_ns_generic[pdp_idx][i] * 1.0e-9 * scale / Ts));

        // linear power value
        const double B = common::adt::db2pow(tap_powers_dB_generic[pdp_idx][i]);

        // is this sample delay already defined?
        int idx = -1;
        for (uint32_t j = 0; j < tap_delays_smpl.size(); ++j) {
            if (tap_delays_smpl[j] == A) {
                idx = static_cast<int>(j);
                break;
            }
        }

        // if not defined yet, add new delay
        if (idx < 0) {
            tap_delays_smpl.push_back(A);
            tap_powers_linear.push_back(B);
        }
        // otherwise accumulate power at the same delay
        else {
            // add power to known delay
            tap_powers_linear[idx] += B;
        }
    }

    // normalize power across all delays to 1.0f
    const double sum = std::accumulate(tap_powers_linear.begin(), tap_powers_linear.end(), 0.0);
    for (uint32_t i = 0; i < tap_powers_linear.size(); ++i) {
        tap_powers_linear[i] /= sum;
    }

    dectnrp_assert(!tap_delays_smpl.empty() && tap_delays_smpl.size() == tap_powers_linear.size(),
                   "Number of delays/powers incorrect.");
    dectnrp_assert(tap_delays_smpl[0] == 0, "First delay must be zero.");
    dectnrp_assert(std::accumulate(tap_powers_linear.begin(), tap_powers_linear.end(), 0.0) > 0.999,
                   "Power smaller 1");
    dectnrp_assert(
        std::accumulate(tap_powers_linear.begin(), tap_powers_linear.end(), 0.0) < 1.0001,
        "Power larger 1");
    for (uint32_t i = 0; i < tap_delays_smpl.size(); ++i) {
        dectnrp_assert(std::isfinite(tap_delays_smpl[i]) && std::isfinite(tap_powers_linear[i]),
                       "Delay of power is not finite.");
    }
    dectnrp_assert(tap_delays_smpl.back() < spp_size, "Delay larger than spp size.");
}

void link_t::print_pdp(const std::string prefix) const {
    std::string info = prefix;

    info += "tau_rms=" + std::to_string(tau_rms_ns) + " ";
    info += "fD_Hz=" + std::to_string(fD_Hz) + " ";

    for (uint32_t i = 0; i < tap_delays_smpl.size(); ++i) {
        info +=
            std::to_string(tap_delays_smpl[i]) + " " + std::to_string(tap_powers_linear[i]) + " ";
    }

    dectnrp_log_inf("{}", info);
}

void link_t::randomize() {
    // time dependent random numbers
    common::randomgen_t randomgen;
    randomgen.shuffle();

    const double samp_rate_d = static_cast<double>(samp_rate);

    /* Clarke's model assumes evenly distributed angles of arrival. Jakes' model assumes randomly
     * distributed angles of arrival according to a uniform distribution.
     */
// #define FOLLOW_CLARKES_MODEL
#ifdef FOLLOW_CLARKES_MODEL
    const double angular_separation = 2.0 * std::numbers::pi_v<double> /
                                      static_cast<double>(WIRELESS_CHANNEL_DOUBLY_NOF_SINUSOIDS);
#endif

    for (uint32_t i = 0; i < tap_delays_smpl.size(); ++i) {
#ifdef FOLLOW_CLARKES_MODEL
        // each tap's sinusoids start at a common random angular offset
        const double random_angular_offset_common =
            randomgen.rand_m1p1() * 2.0 * std::numbers::pi_v<double>;
#endif

        for (uint32_t j = 0; j < WIRELESS_CHANNEL_DOUBLY_NOF_SINUSOIDS; ++j) {
#ifdef FOLLOW_CLARKES_MODEL
            const double angular_offset =
                static_cast<double>(j) * angular_separation + random_angular_offset_common;
#else
            const double angular_offset = randomgen.rand_m1p1() * 2.0 * std::numbers::pi_v<double>;
#endif
            // frequency of this particular sinusoid
            const double fD_Hz_this = fD_Hz * std::cos(angular_offset);

            // frequency within deadband?
            if (-DOPPLER_FD_HZ_DEADBAND < fD_Hz_this && fD_Hz_this < DOPPLER_FD_HZ_DEADBAND) {
                period_smpl[i][j] = INT64_MAX;
            } else {
                period_smpl[i][j] = static_cast<int64_t>(samp_rate_d / fD_Hz_this);
            }

            // every sinusoid starts rotating with a random phase offset
            phase_inital_rad[i][j] = randomgen.rand_m1p1() * 2.0f * std::numbers::pi_v<float>;
        }
    }

    for (auto history_stage : history_stage_arr) {
        srsran_vec_cf_zero(history_stage, spp_size * 2);
    }

    srsran_vec_cf_zero(sinusoid_stage, spp_size);
    srsran_vec_cf_zero(superposition_stage, spp_size);
}

void link_t::pass_through_link(const cf_t* inp,
                               cf_t* out,
                               const bool primary_direction,
                               const int64_t time_64) {
    // zero output, used for superposition
    srsran_vec_cf_zero(out, spp_size);

    cf_t* history_stage = primary_direction ? history_stage_arr[0] : history_stage_arr[1];

    // copy input right next to former samples
    srsran_vec_cf_copy(&history_stage[spp_size], inp, spp_size);

    for (uint32_t i = 0; i < tap_delays_smpl.size(); ++i) {
        // offset of this tap
        const uint32_t offset = spp_size - tap_delays_smpl[i];

        // on the superposition stage, we superimpose the sinusoids of this tap
        srsran_vec_cf_zero(superposition_stage, spp_size);

        for (uint32_t j = 0; j < WIRELESS_CHANNEL_DOUBLY_NOF_SINUSOIDS; ++j) {
            // phase rotation sample to sample
            const double phase_s2s_rad =
                2.0f * float(M_PI) / static_cast<double>(period_smpl[i][j]);

            // current offset within period
            const double period_offset = static_cast<double>(time_64 % llabs(period_smpl[i][j]));

            // phase at time_64
            const float phase_rad =
                static_cast<float>(phase_s2s_rad * period_offset) + phase_inital_rad[i][j];

            // create rotators
            const lv_32fc_t rotA = lv_cmake(std::cos(static_cast<float>(phase_s2s_rad)),
                                            std::sin(static_cast<float>(phase_s2s_rad)));
            lv_32fc_t rotB = lv_cmake(std::cos(phase_rad), std::sin(phase_rad));

            // rotate
            volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t*)sinusoid_stage,
                                             (const lv_32fc_t*)&history_stage[offset],
                                             &rotA,
                                             &rotB,
                                             spp_size);

            // superimpose sinusoids of this tap
            srsran_vec_sum_ccc(sinusoid_stage, superposition_stage, superposition_stage, spp_size);
        }

        /* Each complex sinusoid has an RMS of 1, and therefore a power of 1. If we have 40
         * sinusoids, the total power is 40. We have to scale the sum with a factor 1/sqrt(40) to
         * bring the average power to 1.
         */
        float scale = 1.0f / sqrt(static_cast<float>(WIRELESS_CHANNEL_DOUBLY_NOF_SINUSOIDS));

        // total power of all taps is 1
        scale *= sqrt(tap_powers_linear[i]);

        // scale with the number of sinusoids and the tap power
        srsran_vec_sc_prod_cfc(superposition_stage, scale, superposition_stage, spp_size);

        // superimpose taps
        srsran_vec_sum_ccc(superposition_stage, out, out, spp_size);
    }

    // overwrite history
    srsran_vec_cf_copy(history_stage, inp, spp_size);
}

constexpr double link_t::tap_delays_ns_generic[WIRELESS_CHANNEL_DOUBLY_NOF_PROFILE]
                                              [WIRELESS_CHANNEL_DOUBLY_NOF_TAPS];

constexpr double link_t::tap_powers_dB_generic[WIRELESS_CHANNEL_DOUBLY_NOF_PROFILE]
                                              [WIRELESS_CHANNEL_DOUBLY_NOF_TAPS];

double link_t::get_tau_rms_ns(const double* tap_delays,
                              const double* tap_powers_dB,
                              uint32_t nof_val) {
    // convert to linear values
    std::vector<double> tap_powers_linear(nof_val);
    for (uint32_t i = 0; i < nof_val; ++i) {
        tap_powers_linear[i] = common::adt::db2pow(tap_powers_dB[i]);
    }

    // sum up entire power
    double sum = 0.0;
    for (uint32_t i = 0; i < nof_val; ++i) {
        sum += tap_powers_linear[i];
    }

    // normalize each tap power
    for (uint32_t i = 0; i < nof_val; ++i) {
        tap_powers_linear[i] /= sum;
    }

    // determine tau mean
    double tau_mean = 0.0;
    for (uint32_t i = 0; i < nof_val; ++i) {
        tau_mean += tap_delays[i] * tap_powers_linear[i];
    }

    // calculate tau rms according to https://en.wikipedia.org/wiki/Delay_spread
    double tau_rms_ns = 0.0;
    for (uint32_t i = 0; i < nof_val; ++i) {
        const double A = tap_delays[i] - tau_mean;
        tau_rms_ns += A * A * tap_powers_linear[i];
    }

    return std::sqrt(tau_rms_ns);
}

}  // namespace dectnrp::simulation
