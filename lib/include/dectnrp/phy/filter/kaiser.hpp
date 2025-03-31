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
#include <vector>

namespace dectnrp::phy::filter {

/**
 * \brief generates a Kaiser window in time domain, PassbandFrequency and StopbandFrequency are
 * given in Hz and must be smaller than SampleRate/2
 *
 * \param PassbandFrequency
 * \param StopbandFrequency
 * \param PassbandRipple_dB
 * \param StopbandAttenuation_dB
 * \param SampleRate
 * \param N_force_odd
 * \return
 */
std::vector<float> kaiser(const float PassbandFrequency,
                          const float StopbandFrequency,
                          const float PassbandRipple_dB,
                          const float StopbandAttenuation_dB,
                          const float SampleRate,
                          const bool N_force_odd);

}  // namespace dectnrp::phy::filter
