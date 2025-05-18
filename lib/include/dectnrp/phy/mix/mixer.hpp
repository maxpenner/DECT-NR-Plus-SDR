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

#include <volk/volk.h>

#include <cstdint>
#include <vector>

#include "dectnrp/common/complex.hpp"

namespace dectnrp::phy {

class mixer_t {
    public:
        mixer_t()
            : mixer_t(0.0f, 0.0f){};

        explicit mixer_t(const float phase_rad_, const float phase_increment_rad_) {
            set_phase(phase_rad_);
            set_phase_increment(phase_increment_rad_);
        };

        ~mixer_t() = default;

        mixer_t(const mixer_t&) = delete;
        mixer_t& operator=(const mixer_t&) = delete;
        mixer_t(mixer_t&&) = delete;
        mixer_t& operator=(mixer_t&&) = delete;

        void set_phase(const float phase_rad);
        void set_phase_increment(const float phase_increment_rad);

        float get_phase() const { return std::arg(phase); }
        float get_phase_increment() const { return std::arg(phase_increment); }

        /// in case we adjust the CFO between consecutive OFDM symbols
        void adjust_phase_increment(const float phase_increment_adjustment_rad);

        /// phase is continuous for consecutive calls
        void mix_phase_continuous(const std::vector<cf_t*>& in,
                                  const std::vector<cf_t*> out,
                                  const uint32_t nof_samples);

        void mix_phase_continuous_offset(const std::vector<cf_t*>& in,
                                         const uint32_t offset_in,
                                         const std::vector<cf_t*> out,
                                         const uint32_t offset_out,
                                         const uint32_t nof_samples);

        /// phase is continuous for consecutive calls, but we don't actually mix
        void skip_phase_continuous(const uint32_t nof_samples);

    private:
        lv_32fc_t phase;
        lv_32fc_t phase_increment;
};

}  // namespace dectnrp::phy
