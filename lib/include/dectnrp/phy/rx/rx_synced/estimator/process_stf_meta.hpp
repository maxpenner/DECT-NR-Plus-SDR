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

namespace dectnrp::phy {

/**
 * \brief Instances of this class contain meta information about an OFDM symbol with STF
 * cells. Each deriving class can use that information as needed.
 */
class process_stf_meta_t {
    public:
        process_stf_meta_t(const int64_t fine_peak_time_64_)
            : fine_peak_time_64(fine_peak_time_64_) {}

        process_stf_meta_t() = delete;
        process_stf_meta_t(const process_stf_meta_t&) = delete;
        process_stf_meta_t& operator=(const process_stf_meta_t&) = delete;
        process_stf_meta_t(process_stf_meta_t&&) = delete;
        process_stf_meta_t& operator=(process_stf_meta_t&&) = delete;

        const int64_t fine_peak_time_64;
};

}  // namespace dectnrp::phy
