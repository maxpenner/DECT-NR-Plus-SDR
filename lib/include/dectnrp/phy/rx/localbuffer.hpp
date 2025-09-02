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

#include <cstdint>
#include <vector>

#include "dectnrp/common/complex.hpp"

namespace dectnrp::phy {

class localbuffer_t {
    public:
        explicit localbuffer_t(const uint32_t nof_antennas_limited_, const uint32_t nof_samples_);
        ~localbuffer_t();

        localbuffer_t() = delete;
        localbuffer_t(const localbuffer_t&) = delete;
        localbuffer_t& operator=(const localbuffer_t&) = delete;
        localbuffer_t(localbuffer_t&&) = delete;
        localbuffer_t& operator=(localbuffer_t&&) = delete;

        const uint32_t nof_antennas_limited;
        const uint32_t nof_samples;

        friend class rx_pacer_t;

    private:
        /// time of first sample
        int64_t ant_streams_time_64;

        /// number of samples written
        uint32_t cnt_w;

        std::vector<cf_t*> buffer_vec;
};

}  // namespace dectnrp::phy
