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

#include "dectnrp/simulation/hardware/noise.hpp"

#include <cmath>

namespace dectnrp::simulation {

noise_t::noise_t(const uint32_t seed_) { srsran_channel_awgn_init(&srsran_channel_awgn, seed_); }

noise_t::~noise_t() { srsran_channel_awgn_free(&srsran_channel_awgn); }

void noise_t::awgn(const cf_t* inp,
                   cf_t* out,
                   const uint32_t nof_samples,
                   const float net_bandwidth_norm,
                   const float snr_in_net_bandwidth_norm_dB) {
    // https://github.com/srsran/srsRAN_4G/blob/master/lib/src/phy/channel/ch_awgn.c#L108
    srsran_channel_awgn_set_n0(&srsran_channel_awgn,
                               -10.0f * log10(net_bandwidth_norm) - snr_in_net_bandwidth_norm_dB);

    srsran_channel_awgn_run_c(&srsran_channel_awgn, inp, out, nof_samples);
}

}  // namespace dectnrp::simulation
