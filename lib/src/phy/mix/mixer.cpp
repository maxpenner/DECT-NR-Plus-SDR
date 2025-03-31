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

#include "dectnrp/phy/mix/mixer.hpp"

#include <cmath>
#include <complex>

namespace dectnrp::phy {

void mixer_t::set_phase(const float phase_rad) {
    phase = lv_cmake(std::cos(phase_rad), std::sin(phase_rad));
}

void mixer_t::set_phase_increment(const float phase_increment_rad) {
    phase_increment = lv_cmake(std::cos(phase_increment_rad), std::sin(phase_increment_rad));
}

void mixer_t::adjust_phase_increment(const float phase_increment_adjustment_rad) {
    phase_increment = phase_increment * lv_cmake(std::cos(phase_increment_adjustment_rad),
                                                 std::sin(phase_increment_adjustment_rad));
}

void mixer_t::mix_phase_continuous(const std::vector<cf_t*>& in,
                                   const std::vector<cf_t*> out,
                                   const uint32_t nof_samples) {
    mix_phase_continuous_offset(in, 0, out, 0, nof_samples);
}

void mixer_t::mix_phase_continuous_offset(const std::vector<cf_t*>& in,
                                          const uint32_t offset_in,
                                          const std::vector<cf_t*> out,
                                          const uint32_t offset_out,
                                          const uint32_t nof_samples) {
    lv_32fc_t phase_local_copy;

    for (uint32_t i = 0; i < in.size(); ++i) {
        phase_local_copy = phase;
        volk_32fc_s32fc_x2_rotator2_32fc((lv_32fc_t*)&out[i][offset_out],
                                         (const lv_32fc_t*)&in[i][offset_in],
                                         &phase_increment,
                                         &phase_local_copy,
                                         nof_samples);
    }

    // overwrite with value from last call
    phase = phase_local_copy;
}

void mixer_t::skip_phase_continuous(const uint32_t nof_samples) {
    float angle_rad = std::arg(phase_increment);
    angle_rad *= static_cast<float>(nof_samples);
    phase *= lv_cmake(std::cos(angle_rad), std::sin(angle_rad));
}

}  // namespace dectnrp::phy
