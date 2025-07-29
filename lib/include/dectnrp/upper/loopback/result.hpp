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

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::upper::tfw::loopback {

class result_t {
    public:
        result_t() = default;

        explicit result_t(const std::size_t N_parameter_values, const std::size_t N_snr_values);

        /// at every SNR the success metrics are CRC hits and the PLCF content
        uint32_t n_pcc;
        uint32_t n_pcc_and_plcf;
        uint32_t n_pdc;

        /// at every SNR we also save the measured SNR of the PDC
        common::vec2d<float> snr_max_vec;
        common::vec2d<float> snr_min_vec;

        void overwrite_or_discard_snr_max(const uint32_t row,
                                          const uint32_t col,
                                          const float snr_max);

        void overwrite_or_discard_snr_min(const uint32_t row,
                                          const uint32_t col,
                                          const float snr_min);

        void set_PERs(const uint32_t row, const uint32_t col, const float snr_min);

        /// global results container to later write results to file
        common::vec2d<float> PER_pcc;
        common::vec2d<float> PER_pcc_and_plcf;
        common::vec2d<float> PER_pdc;

        void reset();
};

}  // namespace dectnrp::upper::tfw::loopback
