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

#include "dectnrp/sections_part4/mac_messages_and_ie/neighbouring_ie.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/sections_part2/channel_arrangement.hpp"

namespace dectnrp::sp4 {

neighbouring_ie_t::neighbouring_ie_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Neighbouring_IE;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void neighbouring_ie_t::zero() {
    short_rd_id = common::adt::UNDEFINED_NUMERIC_32;

    radio_device_class = {.mu = rd_capability_ie_t::subcarrier_width_t::not_defined,
                          .beta = rd_capability_ie_t::dft_size_t::not_defined};
    radio_device_class.reset();

    measurement_result_snr = common::adt::UNDEFINED_NUMERIC_32;
    measurement_result_snr.reset();

    measurement_result_rssi_2 = common::adt::UNDEFINED_NUMERIC_32;
    measurement_result_rssi_2.reset();

    has_power_constraints = false;

    next_cluster_channel = common::adt::UNDEFINED_NUMERIC_32;
    next_cluster_channel.reset();

    time_to_next = common::adt::UNDEFINED_NUMERIC_32;
    time_to_next.reset();

    network_beacon_period = network_beacon_message_t::network_beacon_period_t::not_defined;
    cluster_beacon_period = network_beacon_message_t::cluster_beacon_period_t::not_defined;
}

bool neighbouring_ie_t::is_valid() const {
    if (short_rd_id > common::adt::bitmask_lsb<16>()) {
        return false;
    }

    if (const auto& value = radio_device_class.value();
        radio_device_class.has_value() &&
        (!common::adt::is_valid(value.mu) || !common::adt::is_valid(value.beta))) {
        return false;
    }

    if (measurement_result_snr.has_value() && measurement_result_snr.value() > 0xff) {
        return false;
    }

    if (measurement_result_rssi_2.has_value() && measurement_result_rssi_2.value() > 0xff) {
        return false;
    }

    if (next_cluster_channel.has_value() &&
        !sp2::is_absolute_channel_number_in_range(next_cluster_channel.value())) {
        return false;
    }

    if (!common::adt::is_valid(network_beacon_period) ||
        !common::adt::is_valid(cluster_beacon_period)) {
        return false;
    }

    return true;
}

uint32_t neighbouring_ie_t::get_packed_size() const {
    uint32_t packed_size = 4;
    packed_size += radio_device_class.has_value();
    packed_size += measurement_result_snr.has_value();
    packed_size += measurement_result_rssi_2.has_value();
    packed_size += next_cluster_channel.has_value() * 2;
    packed_size += time_to_next.has_value() * 4;
    return packed_size;
}

void neighbouring_ie_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that Neighbouring IE is valid before packing
    dectnrp_assert(is_valid(), "Neighbouring IE is not valid");

    // set SHORT RD-ID in octets 0-1
    common::adt::l2b_lower(mac_pdu_offset, short_rd_id, 2);

    // set non-optional fields in octet 2
    mac_pdu_offset[2] = radio_device_class.has_value() << 5;
    mac_pdu_offset[2] |= measurement_result_snr.has_value() << 4;
    mac_pdu_offset[2] |= measurement_result_rssi_2.has_value() << 3;
    mac_pdu_offset[2] |= has_power_constraints << 2;
    mac_pdu_offset[2] |= next_cluster_channel.has_value() << 1;
    mac_pdu_offset[2] |= time_to_next.has_value();

    // set non-optional fields in octet 3
    mac_pdu_offset[3] = std::to_underlying(network_beacon_period) << 4;
    mac_pdu_offset[3] |= std::to_underlying(cluster_beacon_period);

    uint32_t N_bytes = 4;

    // pack NEXT CLUSTER CHANNEL field (if present)
    if (const auto& value = next_cluster_channel.value(); next_cluster_channel.has_value()) {
        *(mac_pdu_offset + N_bytes) = value >> 5;
        *(mac_pdu_offset + N_bytes + 1) = value & 0xff;
        N_bytes += 2;
    }

    // pack TIME TO NEXT field (if present)
    if (time_to_next.has_value()) {
        common::adt::l2b_lower(mac_pdu_offset + N_bytes, time_to_next.value(), 4);
        N_bytes += 4;
    }

    // pack RSSI-2 field (if present)
    if (measurement_result_rssi_2.has_value()) {
        mac_pdu_offset[N_bytes] = measurement_result_rssi_2.value();
        ++N_bytes;
    }

    // pack SNR field (if present)
    if (measurement_result_snr.has_value()) {
        mac_pdu_offset[N_bytes] = measurement_result_snr.value();
        ++N_bytes;
    }

    // pack MU and BETA fields (if present)
    if (const auto& value = radio_device_class.value(); radio_device_class.has_value()) {
        mac_pdu_offset[N_bytes] = std::to_underlying(value.mu) << 5;
        mac_pdu_offset[N_bytes] |= std::to_underlying(value.beta) << 1;
        ++N_bytes;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == N_bytes,
                   "lengths do not match");
}

bool neighbouring_ie_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack SHORT RD-ID in octets 0-1
    short_rd_id = common::adt::b2l_lower(mac_pdu_offset, 2);

    // unpack POWER CONST field in octet 2
    has_power_constraints = (mac_pdu_offset[2] >> 2) & 1;

    // unpack non-optional fields in octet 3
    network_beacon_period =
        common::adt::from_coded_value<network_beacon_message_t::network_beacon_period_t>(
            mac_pdu_offset[3] & 0xf0);
    cluster_beacon_period =
        common::adt::from_coded_value<network_beacon_message_t::cluster_beacon_period_t>(
            mac_pdu_offset[3] & 0xf);

    uint32_t N_bytes = 4;

    // unpack NEXT CLUSTER CHANNEL field (if present)
    if (mac_pdu_offset[2] & 0b10) {
        uint32_t value = (mac_pdu_offset[N_bytes] & 0x1f) << 8;
        value |= *(mac_pdu_offset + N_bytes + 1);
        next_cluster_channel = value;
        N_bytes += 2;
    }

    // unpack TIME TO NEXT field (if present)
    if (mac_pdu_offset[2] & 1) {
        time_to_next = common::adt::b2l_lower(mac_pdu_offset + N_bytes, 4);
        N_bytes += 4;
    }

    // unpack RSSI-2 field (if present)
    if ((mac_pdu_offset[2] >> 3) & 1) {
        measurement_result_rssi_2 = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack SNR field (if present)
    if ((mac_pdu_offset[2] >> 4) & 1) {
        measurement_result_snr = mac_pdu_offset[N_bytes];
        ++N_bytes;
    }

    // unpack MU and BETA fields (if present)
    if ((mac_pdu_offset[2] >> 5) & 1) {
        rd_capability_ie_t::radio_device_class_t value;
        value.mu = common::adt::from_coded_value<rd_capability_ie_t::subcarrier_width_t>(
            (mac_pdu_offset[N_bytes] >> 5) & 0b111);
        value.beta = common::adt::from_coded_value<rd_capability_ie_t::dft_size_t>(
            (mac_pdu_offset[N_bytes] >> 1) & 0xf);
        radio_device_class = value;
        ++N_bytes;
    }

    dectnrp_assert(get_packed_size() == N_bytes, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t neighbouring_ie_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t packed_size = 4;
    packed_size += (mac_pdu_offset[2] >> 5) & 1;
    packed_size += (mac_pdu_offset[2] >> 4) & 1;
    packed_size += (mac_pdu_offset[2] >> 3) & 1;
    packed_size += (mac_pdu_offset[2] & 0b10) * 2;
    packed_size += (mac_pdu_offset[2] & 1) * 4;
    return packed_size;
}

};  // namespace dectnrp::sp4
