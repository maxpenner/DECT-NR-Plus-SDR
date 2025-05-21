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

#include "dectnrp/sections_part3/radio_device_class.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp3 {

radio_device_class_t get_radio_device_class(const std::string& radio_device_class_string) {
    radio_device_class_t q;

    if (radio_device_class_string.compare("1.1.1.A") == 0) {
        q.u_min = 1;
        q.b_min = 1;
        q.N_TX_min = 1;
        q.mcs_index_min = 7;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 25344;
        q.Z_min = 2048;

        // set to 16 to be standard compliant
        q.PacketLength_min = 4;

    } else if (radio_device_class_string.compare("1.1.1.B") == 0) {
        q.u_min = 1;
        q.b_min = 1;
        q.N_TX_min = 1;
        q.mcs_index_min = 7;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 25344;
        q.Z_min = 6144;

        // set to 16 to be standard compliant
        q.PacketLength_min = 4;

    } else if (radio_device_class_string.compare("8.1.1.A") == 0) {
        q.u_min = 8;
        q.b_min = 1;
        q.N_TX_min = 1;
        q.mcs_index_min = 7;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 25344;
        q.Z_min = 6144;

        // set to 16 to be standard compliant
        q.PacketLength_min = 4;

    } else if (radio_device_class_string.compare("1.8.1.A") == 0) {
        q.u_min = 1;
        q.b_min = 8;
        q.N_TX_min = 1;
        q.mcs_index_min = 7;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 25344;
        q.Z_min = 6144;

        // set to 16 to be standard compliant
        q.PacketLength_min = 4;

    } else if (radio_device_class_string.compare("2.8.2.A") == 0) {
        q.u_min = 2;
        q.b_min = 8;
        q.N_TX_min = 2;
        q.mcs_index_min = 7;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 25344;
        q.Z_min = 6144;

        // set to 16 to be standard compliant
        q.PacketLength_min = 4;

    } else if (radio_device_class_string.compare("2.12.4.A") == 0) {
        q.u_min = 2;
        q.b_min = 12;
        q.N_TX_min = 4;
        q.mcs_index_min = 7;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 25344;
        q.Z_min = 2048;

        // set to 16 to be standard compliant
        q.PacketLength_min = 4;

    } else if (radio_device_class_string.compare("2.12.4.B") == 0) {
        q.u_min = 2;
        q.b_min = 12;
        q.N_TX_min = 4;
        q.mcs_index_min = 7;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 25344;
        q.Z_min = 6144;

        // set to 16 to be standard compliant
        q.PacketLength_min = 4;

    } else if (radio_device_class_string.compare("8.12.8.A") == 0) {
        q.u_min = 8;
        q.b_min = 12;
        q.N_TX_min = 8;
        q.mcs_index_min = 9;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 225344;
        q.Z_min = 6144;

        // set to 16 to be standard compliant
        q.PacketLength_min = 16;

    } else if (radio_device_class_string.compare("8.16.8.A") == 0) {
        q.u_min = 8;
        q.b_min = 16;
        q.N_TX_min = 8;
        q.mcs_index_min = 9;
        q.M_DL_HARQ_min = 8;
        q.M_connection_DL_HARQ_min = 2;
        q.N_soft_min = 225344;
        q.Z_min = 6144;

        // set to 16 to be standard compliant
        q.PacketLength_min = 16;

    } else {
        dectnrp_assert_failure("Unknown Radio Device Class {}", radio_device_class_string);
    }

    return q;
}

}  // namespace dectnrp::sp3
