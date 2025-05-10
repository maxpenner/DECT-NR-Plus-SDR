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

#include "dectnrp/common/complex.hpp"
#include "dectnrp/phy/rx/chscan/chscan.hpp"
#include "dectnrp/radio/buffer_rx.hpp"
#include "dectnrp/sections_part3/derivative/duration_lut.hpp"

namespace dectnrp::phy {

class chscanner_t {
    public:
        explicit chscanner_t(const radio::buffer_rx_t& buffer_rx_);
        ~chscanner_t() = default;

        chscanner_t() = delete;
        chscanner_t(const chscanner_t&) = delete;
        chscanner_t& operator=(const chscanner_t&) = delete;
        chscanner_t(chscanner_t&&) = delete;
        chscanner_t& operator=(chscanner_t&&) = delete;

        void scan(chscan_t& ch_scan);

    private:
        const radio::buffer_rx_t& buffer_rx;          // required to poll time
        const std::vector<const cf_t*> ant_streams;   // direct access to IQ samples
        const uint32_t ant_streams_length_samples;    // number of buffered IQ samples
        const section3::duration_lut_t duration_lut;  // used to determine length of partial scan

        /// number of antennas we use for each partial scan
        uint32_t N_ant;

        float scan_partial(const int64_t start_64, const int64_t length_64);
};

}  // namespace dectnrp::phy
