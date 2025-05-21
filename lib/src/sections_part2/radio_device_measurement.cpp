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

#include "dectnrp/sections_part2/radio_device_measurement.hpp"

#include <cmath>

namespace dectnrp::sp2 {

rssi_x_measurement_report_t get_rssi_x_measurement_report(const float measured_value_dBm) {
    rssi_x_measurement_report_t rssi_x_measurement_report;

    rssi_x_measurement_report.measured_value_dBm = measured_value_dBm;

    if (-20.5f < measured_value_dBm) {
        rssi_x_measurement_report.reported_value = 1;
    } else {
        rssi_x_measurement_report.reported_value =
            2 + static_cast<uint32_t>(std::floor((-20.5f - measured_value_dBm) / 0.5f));
    }

    if (rssi_x_measurement_report.reported_value > 182) {
        rssi_x_measurement_report.reported_value = 182;
    }

    return rssi_x_measurement_report;
}

snr_measurement_report_t get_snr_measurement_report_t(const float measured_value_dB) {
    snr_measurement_report_t snr_measurement_report;

    snr_measurement_report.measured_value_dB = measured_value_dB;

    if (-4.75f > measured_value_dB) {
        snr_measurement_report.reported_value = 1;
    } else {
        snr_measurement_report.reported_value =
            2 + static_cast<uint32_t>(std::floor((4.75f + measured_value_dB) / 0.25f));
    }

    if (snr_measurement_report.reported_value > 201) {
        snr_measurement_report.reported_value = 201;
    }

    return snr_measurement_report;
}

}  // namespace dectnrp::sp2
