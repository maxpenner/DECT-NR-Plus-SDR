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

extern "C" {
#include "srsran/config.h"
#include "srsran/phy/channel/ch_awgn.h"
}

namespace dectnrp::simulation {

class noise_t {
    public:
        explicit noise_t(const uint32_t seed_);
        ~noise_t();

        noise_t() = delete;
        noise_t(const noise_t&) = delete;
        noise_t& operator=(const noise_t&) = delete;
        noise_t(noise_t&&) = delete;
        noise_t& operator=(noise_t&&) = delete;

        void awgn(const cf_t* inp,
                  cf_t* out,
                  const uint32_t nof_samples,
                  const float net_bandwidth_norm,
                  const float snr_in_net_bandwidth_norm_dB);

    private:
        srsran_channel_awgn_t srsran_channel_awgn;
};

}  // namespace dectnrp::simulation
