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
#include <vector>

#include "dectnrp/sections_part3/derivative/duration_ec.hpp"

namespace dectnrp::phy {

class chscan_t {
    public:
        /**
         * \brief
         *
         * \param end_64_ time of final sample to scan at
         * \param duration_ec_ length of partial scan
         * \param N_partial_ number of partial scans
         * \param N_ant_ maximum number of antennas to use for each partial scan
         */
        explicit chscan_t(const int64_t end_64_,
                          const sp3::duration_ec_t duration_ec_,
                          const uint32_t N_partial_,
                          const uint32_t N_ant_ = std::numeric_limits<uint32_t>::max())
            : end_64(end_64_),
              duration_ec(duration_ec_),
              N_partial(N_partial_),
              N_ant(N_ant_) {}

        std::vector<float> get_rms_vec() const { return rms_vec; }
        float get_rms_avg() const { return rms_avg; }

        friend class chscanner_t;

    private:
        int64_t end_64;
        sp3::duration_ec_t duration_ec;
        uint32_t N_partial;
        uint32_t N_ant;

        /// we keep one RMS value for each partial scan ...
        std::vector<float> rms_vec;

        /// ... and one average RMS
        float rms_avg{0.0f};
};

}  // namespace dectnrp::phy
