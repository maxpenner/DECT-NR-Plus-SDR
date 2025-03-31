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

namespace dectnrp::section3 {

/**
 * \brief Meta information of a DECT NR+ packet that are not strictly related to the DECT NR+
 * standard. An instance of this structure is member of tx_descriptor_t, which contains the full
 * configuration.
 */
using tx_meta_t = struct tx_meta_t {
        /// determines power and ADC dynamic range
        bool optimal_scaling_DAC;
        float DAC_scale;

        /**
         * \brief TX can mix IQ samples in time domain up and down. We pass the initial phase and
         * the sample to sample (s2s) phase increment as desired AFTER resampling, both in radians.
         */
        float iq_phase_rad;
        float iq_phase_increment_s2s_post_resampling_rad;

        /**
         * \brief Value can never be 0 as there must be at least one zero sample. Otherwise we stop
         * transmission at an arbitrary amplitude and might a transient response when using real
         * hardware.
         *
         * The minimum number of samples in the GI is reached for critical sampling at the lowest
         * sample rate possible.
         *      18.52us * 1.728MS/s = 32S
         *
         * Minimum percentage then is:
         *      32 * 3/100 = 0.96 < 1
         *      32 * 4/100 = 1.28 > 1
         */
        uint32_t GI_percentage;
};

}  // namespace dectnrp::section3
