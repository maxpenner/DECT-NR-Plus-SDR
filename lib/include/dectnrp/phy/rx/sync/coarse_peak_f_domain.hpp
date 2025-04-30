/*
 * Copyright 2023-2024 Maxim Penner, Mattes Wassmann
 * Copyright 2025-2025 Maxim Penner
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
#include <utility>
#include <vector>

#include "dectnrp/common/complex.hpp"
#include "dectnrp/phy/dft/ofdm.hpp"

namespace dectnrp::phy {

class coarse_peak_f_domain_t {
    public:
        /**
         * \brief This class estimates beta and the integer CFO at the coarse peak in frequency
         * domain. Both values must be known before crosscorrelating with STF templates.
         *
         * \param localbuffer_
         * \param u_max_
         * \param b_max_
         * \param nof_antennas_limited_
         * \param bos_fac_
         */
        explicit coarse_peak_f_domain_t(const std::vector<cf_t*> localbuffer_,
                                        const uint32_t u_max_,
                                        const uint32_t b_max_,
                                        const uint32_t nof_antennas_limited_,
                                        const uint32_t bos_fac_);
        ~coarse_peak_f_domain_t();

        coarse_peak_f_domain_t() = delete;
        coarse_peak_f_domain_t(const coarse_peak_f_domain_t&) = delete;
        coarse_peak_f_domain_t& operator=(const coarse_peak_f_domain_t&) = delete;
        coarse_peak_f_domain_t(coarse_peak_f_domain_t&&) = delete;
        coarse_peak_f_domain_t& operator=(coarse_peak_f_domain_t&&) = delete;

        /// first value is the estimated value of beta, the second value the integer CFO in radians
        std::pair<uint32_t, float> process(const uint32_t coarse_peak_time_local);

    private:
        /// read-only buffer provided by pacer
        const std::vector<cf_t*> localbuffer;

        const uint32_t b_max;
        const uint32_t nof_antennas_limited;

        /// length of cyclic prefix of STF at u_max
        const uint32_t N_samples_STF_CP_only_os;

        /// used to transform into frequency domain ...
        dft::ofdm_t ofdm;

        /// ... onto the fft_stage
        std::vector<cf_t*> fft_stage;

        uint32_t estimate_beta() const;
        float estimate_integer_cfo() const;
};

}  // namespace dectnrp::phy
