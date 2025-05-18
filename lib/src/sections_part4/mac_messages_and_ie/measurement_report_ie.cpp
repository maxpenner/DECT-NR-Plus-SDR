/*
 * Copyright 2023-2024 Maxim Penner, Alexander Poets
 * Copyright 2025-2025 Maxim Penner
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

#include "dectnrp/sections_part4/mac_messages_and_ie/measurement_report_ie.hpp"

#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::section4 {

measurement_report_ie_t::measurement_report_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Measurement_Report_IE;

    zero();

    dectnrp_assert(check_validity_at_runtime(this), "mmie invalid");
}

void measurement_report_ie_t::zero() {
    measurement_result_snr = common::adt::UNDEFINED_NUMERIC_32;
    measurement_result_snr.reset();

    measurement_result_rssi_2 = common::adt::UNDEFINED_NUMERIC_32;
    measurement_result_rssi_2.reset();

    measurement_result_rssi_1 = common::adt::UNDEFINED_NUMERIC_32;
    measurement_result_rssi_1.reset();

    measurement_result_tx_count = common::adt::UNDEFINED_NUMERIC_32;
    measurement_result_tx_count.reset();

    rach = measurement_result_source_t::not_defined;
}

bool measurement_report_ie_t::is_valid() const {
    if (measurement_result_snr.has_value() && measurement_result_snr.value() > 0xff) {
        return false;
    }

    if (measurement_result_rssi_2.has_value() && measurement_result_rssi_2.value() > 0xff) {
        return false;
    }

    if (measurement_result_rssi_1.has_value() && measurement_result_rssi_1.value() > 0xff) {
        return false;
    }

    if (measurement_result_tx_count.has_value() && measurement_result_tx_count.value() > 0xff) {
        return false;
    }

    return common::adt::is_valid(rach);
}

uint32_t measurement_report_ie_t::get_packed_size() const {
    uint32_t packed_size = 1;
    packed_size += measurement_result_snr.has_value();
    packed_size += measurement_result_rssi_2.has_value();
    packed_size += measurement_result_rssi_1.has_value();
    packed_size += measurement_result_tx_count.has_value();
    return packed_size;
}

void measurement_report_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that Measurement Report IE is valid before packing
    dectnrp_assert(is_valid(), "Measurement Report IE is not valid");

    // pack mandatory fields in octet 0 of IE
    mac_pdu_offset[0] = measurement_result_snr.has_value() << 4;
    mac_pdu_offset[0] |= measurement_result_rssi_2.has_value() << 3;
    mac_pdu_offset[0] |= measurement_result_rssi_1.has_value() << 2;
    mac_pdu_offset[0] |= measurement_result_tx_count.has_value() << 1;
    mac_pdu_offset[0] |= std::to_underlying(rach);

    uint32_t N_bytes = 1;

    // pack SNR RESULT field (if included)
    if (measurement_result_snr.has_value()) {
        mac_pdu_offset[N_bytes] = measurement_result_snr.value();
        ++N_bytes;
    }

    // pack RSSI-2 RESULT field (if included)
    if (measurement_result_rssi_2.has_value()) {
        mac_pdu_offset[N_bytes] = measurement_result_rssi_2.value();
        ++N_bytes;
    }

    // pack RSSI-1 RESULT field (if included)
    if (measurement_result_rssi_1.has_value()) {
        mac_pdu_offset[N_bytes] = measurement_result_rssi_1.value();
        ++N_bytes;
    }

    // pack TX COUNT RESULT field (if included)
    if (measurement_result_tx_count.has_value()) {
        mac_pdu_offset[N_bytes] = measurement_result_tx_count.value();
        ++N_bytes;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == N_bytes,
                   "lengths do not match");
}

bool measurement_report_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack RACH field in octet 0 of IE
    rach = common::adt::from_coded_value<measurement_result_source_t>(mac_pdu_offset[0] & 1);

    uint32_t N_bytes = 1;

    // unpack SNR RESULT field (if present)
    if ((mac_pdu_offset[0] >> 4) & 1) {
        measurement_result_snr = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack RSSI-2 RESULT field (if present)
    if ((mac_pdu_offset[0] >> 3) & 1) {
        measurement_result_rssi_2 = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack RSSI-1 RESULT field (if present)
    if ((mac_pdu_offset[0] >> 2) & 1) {
        measurement_result_rssi_1 = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack TX COUNT RESULT field (if present)
    if (mac_pdu_offset[0] & 0b10) {
        measurement_result_tx_count = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    dectnrp_assert(get_packed_size() == N_bytes, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t measurement_report_ie_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t packed_size = 1;
    packed_size += (mac_pdu_offset[0] >> 4) & 1;
    packed_size += (mac_pdu_offset[0] >> 3) & 1;
    packed_size += (mac_pdu_offset[0] >> 2) & 1;
    packed_size += mac_pdu_offset[0] & 0b10;
    return packed_size;
}

}  // namespace dectnrp::section4
