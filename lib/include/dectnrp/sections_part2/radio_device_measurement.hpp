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

namespace dectnrp::sp2 {

struct rssi_x_measurement_report_t {
        float measured_value_dBm;
        uint32_t reported_value;
};

constexpr float RSSI_2_leaky_integrator_alpha{0.1f};

struct snr_measurement_report_t {
        float measured_value_dB;
        uint32_t reported_value;
};

rssi_x_measurement_report_t get_rssi_x_measurement_report(const float measured_value_dBm);

snr_measurement_report_t get_snr_measurement_report_t(const float measured_value_dB);

}  // namespace dectnrp::sp2
