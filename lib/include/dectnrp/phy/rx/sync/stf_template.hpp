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

#include <cstdint>
#include <vector>

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/common/multidim.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/sections_part3/stf.hpp"

namespace dectnrp::phy {

class stf_template_t {
    public:
        explicit stf_template_t(const uint32_t u_max_,
                                const uint32_t b_max_,
                                const uint32_t os_min_,
                                const uint32_t N_eff_TX_max_,
                                const resampler_param_t resampler_param_);
        ~stf_template_t();

        stf_template_t() = delete;
        stf_template_t(const stf_template_t&) = delete;
        stf_template_t& operator=(const stf_template_t&) = delete;
        stf_template_t(stf_template_t&&) = delete;
        stf_template_t& operator=(stf_template_t&&) = delete;

        const std::vector<cf_t*>& get_stf_time_domain(const uint32_t b) const;

        const uint32_t stf_bos_length_samples;               // full STF length without resampling
        const uint32_t stf_bos_rs_length_samples;            // full STF length with resampling
        const uint32_t stf_bos_rs_length_effective_samples;  // partial STF length

    private:
        /// frequency domain
        const section3::stf_t stf;

        /// time domain
        common::vec2d<cf_t*> stf_time_domain;

        /**
         * \brief Returned vector contains the STFs in time domain for a particular value of u. It
         * will have up to 6 rows, each containing one matrix. Six values of b and up to 4 values of
         * N_eff_TX. Each STF has the same length.
         *
         * row 0:   [1 to 4][stf_bos_length]
         * row 1:   [1 to 4][stf_bos_length]
         * row 2:   [1 to 4][stf_bos_length]
         * row 3:   [1 to 4][stf_bos_length]
         * row 4:   [1 to 4][stf_bos_length]
         * row 5:   [1 to 4][stf_bos_length]
         *
         * \param u_max
         * \param b_max
         * \param os_min
         * \param N_eff_TX_max
         * \param L
         * \param M
         * \param stf_bos_length_samples_compare
         * \param stf_bos_rs_length_samples_compare
         * \return
         */
        static common::vec2d<cf_t*> generate_stf_time_domain(
            const uint32_t u_max,
            const uint32_t b_max,
            const uint32_t os_min,
            const uint32_t N_eff_TX_max,
            const uint32_t L,
            const uint32_t M,
            const uint32_t stf_bos_length_samples_compare,
            const uint32_t stf_bos_rs_length_samples_compare);
};

}  // namespace dectnrp::phy
