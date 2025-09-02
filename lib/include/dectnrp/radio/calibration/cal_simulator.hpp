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

#include <vector>

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::radio::calibration::simulator {

const static std::vector<float> freqs_Hz = {0.1e9, 6.0e9};

const static common::vec2d<float> gains_tx_dB = {{0.0, 60.0}, {0.0, 60.0}};
const static common::vec2d<float> powers_tx_dBm = {{-40.0, 20.0}, {-40.0, 20.0}};
const static float gains_tx_dB_step = 1.0;

const static common::vec2d<float> gains_rx_dB = {{70.0, 0.0}, {70.0, 0.0}};
const static common::vec2d<float> powers_rx_dBm = {{-60, 10}, {-60, 10}};
const static float gains_rx_dB_step = 1.0;

}  // namespace dectnrp::radio::calibration::simulator
