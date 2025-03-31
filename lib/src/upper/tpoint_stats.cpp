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

#include "dectnrp/upper/tpoint_stats.hpp"

namespace dectnrp::upper {

std::string tpoint_stats_t::get_as_string() const {
    std::string str;

    str += "rx_pcc_success=" + std::to_string(rx_pcc_success) + " ";
    str += "rx_pcc2pdc_success=" + std::to_string(rx_pcc2pdc_success) + " ";
    str += "rx_pcc2pdc_running_success=" + std::to_string(rx_pcc2pdc_running_success) + " ";
    str += "rx_pdc_success=" + std::to_string(rx_pdc_success) + " ";
    str += "rx_pdc_fail=" + std::to_string(rx_pdc_fail) + " ";
    str += "rx_pdc_has_mmie=" + std::to_string(rx_pdc_has_mmie) + " ";
    str += "beacon_cnt=" + std::to_string(beacon_cnt) + " ";

    return str;
}

}  // namespace dectnrp::upper
