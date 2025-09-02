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

namespace dectnrp::phy {

class channel_statistics_t {
    public:
        explicit channel_statistics_t(const double delta_u_f_,
                                      const double T_u_symb_,
                                      const double nu_max_hz_,
                                      const double tau_rms_sec_,
                                      const double snr_db_,
                                      const uint32_t nof_drs_interp_lr_,
                                      const uint32_t nof_drs_interp_l_)
            : delta_u_f(delta_u_f_),
              T_u_symb(T_u_symb_),
              nu_max_hz(nu_max_hz_),
              tau_rms_sec(tau_rms_sec_),
              snr_db(snr_db_),
              sigma(1.0 / std::pow(10.0, snr_db / 10.0)),
              nof_drs_interp_lr(nof_drs_interp_lr_),
              nof_drs_interp_l(nof_drs_interp_l_) {};

        /// subcarrier spacing in Hz
        const double delta_u_f;

        /// OFDM symbol length in seconds, with CP
        const double T_u_symb;

        /// maximum Doppler spread in Hz
        const double nu_max_hz;

        /// RMS of delay spread in sec
        const double tau_rms_sec;

        /// expected value of SNR in dB
        const double snr_db;

        /// noise power in S/sigma when expected value of S is S=1
        const double sigma;

        /// optimized interpolation lengths for the given channel conditions
        const uint32_t nof_drs_interp_lr;
        const uint32_t nof_drs_interp_l;

        /// source: page 28 in f_subc_spacing https://publik.tuwien.ac.at/files/PubDat_204518.pdf
        template <typename T>
        static T get_r_f_exp(const float tau_rms_sec, const float delta_f) {
            return T{1.0f, 0.0f} / T{1.0f, 2.0f * float(M_PI) * tau_rms_sec * delta_f};
        }

        /// source: Two-Dimensional Pilot-Symbol-Aided Channel Estimation By Wiener Filtering
        static float get_r_f_uni(const float tau_rms_sec, const float delta_f);

        /// source: page 28 in f_subc_spacing https://publik.tuwien.ac.at/files/PubDat_204518.pdf
        static float get_r_t_jakes(const float nu_max_hz, const float delta_t);
};

}  // namespace dectnrp::phy
