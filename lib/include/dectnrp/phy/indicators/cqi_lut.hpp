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

#include <array>
#include <cstdint>

namespace dectnrp::phy::indicators {

class cqi_lut_t {
    public:
        cqi_lut_t()
            : cqi_lut_t(0, snr_required.size() - 1, 0.0f) {};
        cqi_lut_t(const uint32_t mcs_min_, const uint32_t mcs_max_, const float snr_offset_);
        ~cqi_lut_t() = default;

        uint32_t get_mcs_min() const { return mcs_min; };
        uint32_t get_mcs_max() const { return mcs_max; };

        uint32_t get_highest_mcs_possible(const float snr_dB_measured) const;

        uint32_t clamp_mcs(const uint32_t mcs_candidate) const;

        float get_snr_at_mcs_min() const { return snr_required[mcs_min]; }
        float get_snr_at_mcs_max() const { return snr_required[mcs_max]; }

    private:
        uint32_t mcs_min;
        uint32_t mcs_max;

        static constexpr std::array<float, 12> snr_required{-1.0f,   // MCS-0  BPSK     1/2
                                                            1.0f,    // MCS-1  QPSK     1/2
                                                            4.0f,    // MCS-2  QPSK     3/4
                                                            7.0f,    // MCS-3  16-QAM   1/2
                                                            11.0f,   // MCS-4  16-QAM   3/4
                                                            14.0f,   // MCS-5  64-QAM   2/3
                                                            15.0f,   // MCS-6  64-QAM   3/4
                                                            17.5f,   // MCS-7  64-QAM   5/6
                                                            21.0f,   // MCS-8  256-QAM  3/4
                                                            24.0f,   // MCS-9  256-QAM  5/6
                                                            27.0f,   // MCS-10 1024-QAM 3/4
                                                            30.0f};  // MCS-11 1024-QAM 5/6

        float snr_offset;
};

}  // namespace dectnrp::phy::indicators
