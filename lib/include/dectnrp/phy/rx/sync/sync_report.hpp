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
#include <limits>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/ant.hpp"

namespace dectnrp::phy {

class sync_report_t {
    public:
        sync_report_t() = default;
        sync_report_t(const uint32_t nof_antennas_limited)
            : coarse_peak_array(common::ant_t(nof_antennas_limited)),
              rms_array(common::ant_t(nof_antennas_limited)){};

        // ##################################################
        // detection

        uint32_t detection_ant_idx{std::numeric_limits<uint32_t>::max()};
        float detection_rms{-1.0f};
        float detection_metric{-1.0f};
        uint32_t detection_time_local{std::numeric_limits<uint32_t>::max()};
        uint32_t detection_time_with_jump_back_local{std::numeric_limits<uint32_t>::max()};

        /// mu
        uint32_t u{0};

        // ##################################################
        // coarse peak

        /// values above 0 indicate antenna has a valid coarse peak
        common::ant_t coarse_peak_array{};

        /// average of individual coarse peak times weighted by coarse peak height
        uint32_t coarse_peak_time_local{std::numeric_limits<uint32_t>::max()};

        /// synchronization measures the RMS only for antennas with a valid coarse peak
        common::ant_t rms_array{};

        /// average of individual fractional CFOs weighted by coarse peak height
        float cfo_fractional_rad{std::numeric_limits<float>::max()};

        /// estimated in frequency domain at coarse peak
        uint32_t b{0};

        /// estimated in frequency domain at coarse peak
        float cfo_integer_rad{std::numeric_limits<float>::max()};

        /// global coarse peak time
        int64_t coarse_peak_time_64{common::adt::UNDEFINED_EARLY_64};

        // ##################################################
        // fine peak

        /**
         * \brief Each values of N_eff_TX stands for a specific STF template. The chosen value of
         * N_eff_TX is the one which maximizes the sum of all fine peak heights across all evaluated
         * antennas.
         */
        uint32_t N_eff_TX{0};

        /// average of individual fine peak times (for N_eff_TX) weighted by fine peak height
        uint32_t fine_peak_time_local{std::numeric_limits<uint32_t>::max()};

        /// global fine peak time, equivalent to an integer Symbol Time Offset (STO)
        int64_t fine_peak_time_64{common::adt::UNDEFINED_EARLY_64};

        // ##################################################
        // determined and overwritten post FFT

        // based on STF and DRS phase rotation in frequency domain
        float sto_fractional{};

        /// may deviate a few samples from fine_peak_time_64
        int64_t fine_peak_time_corrected_by_sto_fractional_64{common::adt::UNDEFINED_EARLY_64};
};

}  // namespace dectnrp::phy
