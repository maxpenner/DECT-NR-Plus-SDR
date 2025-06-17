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

#include "dectnrp/sections_part4/mac_messages_and_ie/network_beacon_message.hpp"

#include <array>
#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/sections_part2/channel_arrangement.hpp"

namespace dectnrp::sp4 {

// transmit power as coded in Table 6.2.1-3b
static constexpr std::array<int32_t, 13> tx_power_table = {
    -13, -6, -3, 0, 3, 6, 10, 14, 19, 23, 26, 29, 32};

network_beacon_message_t::clusters_max_tx_power_t
network_beacon_message_t::clusters_max_tx_power_from_dBm(const int32_t tx_power_dBm) {
    auto lower = std::lower_bound(tx_power_table.begin(), tx_power_table.end(), tx_power_dBm);

    if (lower == tx_power_table.end()) {
        return static_cast<clusters_max_tx_power_t>(tx_power_table.back());
    }

    return static_cast<clusters_max_tx_power_t>(*lower);
}

int32_t network_beacon_message_t::clusters_max_tx_power_to_dBm(
    const clusters_max_tx_power_t tx_power_coded) {
    uint32_t i = std::to_underlying(tx_power_coded) - 3;
    return tx_power_table.at(i);
}

network_beacon_message_t::network_beacon_message_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Network_Beacon_Message;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void network_beacon_message_t::zero() {
    clusters_max_tx_power = clusters_max_tx_power_t::not_defined;
    clusters_max_tx_power.reset();

    has_power_constraints = false;

    current_cluster_channel = common::adt::UNDEFINED_NUMERIC_32;
    current_cluster_channel.reset();

    network_beacon_channel_0 = common::adt::UNDEFINED_NUMERIC_32;
    network_beacon_channel_0.reset();

    network_beacon_channel_1 = common::adt::UNDEFINED_NUMERIC_32;
    network_beacon_channel_1.reset();

    network_beacon_channel_2 = common::adt::UNDEFINED_NUMERIC_32;
    network_beacon_channel_2.reset();

    network_beacon_period = network_beacon_period_t::not_defined;
    cluster_beacon_period = cluster_beacon_period_t::not_defined;

    next_cluster_channel = common::adt::UNDEFINED_NUMERIC_32;

    time_to_next = common::adt::UNDEFINED_NUMERIC_32;
}

bool network_beacon_message_t::is_valid() const {
    if (clusters_max_tx_power.has_value() &&
        !common::adt::is_valid(clusters_max_tx_power.value())) {
        return false;
    }

    if (current_cluster_channel.has_value() &&
        !sp2::is_absolute_channel_number_in_range(current_cluster_channel.value())) {
        return false;
    }

    if (network_beacon_channel_0.has_value() &&
        !sp2::is_absolute_channel_number_in_range(network_beacon_channel_0.value())) {
        return false;
    }

    if (network_beacon_channel_1.has_value() &&
        !sp2::is_absolute_channel_number_in_range(network_beacon_channel_1.value())) {
        return false;
    }

    if (network_beacon_channel_2.has_value() &&
        !sp2::is_absolute_channel_number_in_range(network_beacon_channel_2.value())) {
        return false;
    }

    if (!common::adt::is_valid(network_beacon_period)) {
        return false;
    }

    if (!common::adt::is_valid(cluster_beacon_period)) {
        return false;
    }

    if (!sp2::is_absolute_channel_number_in_range(next_cluster_channel)) {
        return false;
    }

    return true;
}

uint32_t network_beacon_message_t::get_packed_size() const {
    uint32_t size_packed = 8;
    size_packed += clusters_max_tx_power.has_value();
    size_packed += current_cluster_channel.has_value() * 2;
    size_packed += network_beacon_channel_0.has_value() * 2;
    size_packed += network_beacon_channel_1.has_value() * 2;
    size_packed += network_beacon_channel_2.has_value() * 2;
    return size_packed;
}

void network_beacon_message_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that beacon message is valid before packing
    dectnrp_assert(is_valid(), "network beacon message is not valid");

    /////////////////////////////////
    // set required fields in octet 0
    /////////////////////////////////

    // set TX Power field
    mac_pdu_offset[0] = clusters_max_tx_power.has_value() << 4;

    // set Power Const field
    mac_pdu_offset[0] |= has_power_constraints << 3;

    // set Current field
    mac_pdu_offset[0] |= current_cluster_channel.has_value() << 2;

    // set Network Beacon Channels field
    mac_pdu_offset[0] |= network_beacon_channel_0.has_value() +
                         network_beacon_channel_1.has_value() +
                         network_beacon_channel_2.has_value();

    /////////////////////////////////
    // set required fields in octet 1
    /////////////////////////////////

    // set Network Beacon Period field
    mac_pdu_offset[1] = std::to_underlying(network_beacon_period) << 4;

    // set Cluster Beacon Period field
    mac_pdu_offset[1] |= std::to_underlying(cluster_beacon_period);

    ////////////////////////////////////////
    // set required fields in octets 2 and 3
    ////////////////////////////////////////

    // set Next Cluster Channel field
    mac_pdu_offset[2] = next_cluster_channel >> 8;
    mac_pdu_offset[3] = next_cluster_channel & common::adt::bitmask_lsb<8>();

    ///////////////////////////////////////
    // set required fields in octets 4 to 7
    ///////////////////////////////////////

    // set Time To Next field
    common::adt::l2b_lower(&mac_pdu_offset[4], time_to_next, 4);

    /////////////////////////////////
    // set optional fields if present
    /////////////////////////////////

    uint32_t offset = 8;

    // if included, set optional field Clusters Max TX Power
    if (clusters_max_tx_power.has_value()) {
        mac_pdu_offset[offset] = std::to_underlying(clusters_max_tx_power.value());
        ++offset;
    }

    // if included, set optional field Current Cluster Channel
    if (current_cluster_channel.has_value()) {
        uint32_t temp = current_cluster_channel.value();
        mac_pdu_offset[offset] = temp >> 8;
        mac_pdu_offset[offset + 1] = temp & common::adt::bitmask_lsb<8>();
        offset += 2;
    }

    // if included, set optional field Additional Network Beacon Channel (0)
    if (network_beacon_channel_0.has_value()) {
        uint32_t temp = network_beacon_channel_0.value();
        mac_pdu_offset[offset] = temp >> 8;
        mac_pdu_offset[offset + 1] = temp & common::adt::bitmask_lsb<8>();
        offset += 2;
    }

    // if included, set optional field Additional Network Beacon Channel (1)
    if (network_beacon_channel_1.has_value()) {
        uint32_t temp = network_beacon_channel_1.value();
        mac_pdu_offset[offset] = temp >> 8;
        mac_pdu_offset[offset + 1] = temp & common::adt::bitmask_lsb<8>();
        offset += 2;
    }

    // if included, set optional field Additional Network Beacon Channel (2)
    if (network_beacon_channel_2.has_value()) {
        uint32_t temp = network_beacon_channel_2.value();
        mac_pdu_offset[offset] = temp >> 8;
        mac_pdu_offset[offset + 1] = temp & common::adt::bitmask_lsb<8>();
        offset += 2;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == offset,
                   "lengths do not match");
}

bool network_beacon_message_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    ////////////////////////////////////
    // unpack required fields in octet 0
    ////////////////////////////////////

    // unpack TX Power field
    bool tx_power = (mac_pdu_offset[0] >> 4) & 1;

    // unpack Power Const field
    has_power_constraints = (mac_pdu_offset[0] >> 3) & 1;

    // unpack Current field
    bool current = (mac_pdu_offset[0] >> 2) & 1;

    // unpack Network Beacon Channels field
    bool include_network_beacon_channel_0 = false;
    bool include_network_beacon_channel_1 = false;
    bool include_network_beacon_channel_2 = false;
    switch (mac_pdu_offset[0] & common::adt::bitmask_lsb<2>()) {
        case 3:
            include_network_beacon_channel_2 = true;
            [[fallthrough]];
        case 2:
            include_network_beacon_channel_1 = true;
            [[fallthrough]];
        case 1:
            include_network_beacon_channel_0 = true;
            break;
    }

    ////////////////////////////////////
    // unpack required fields in octet 1
    ////////////////////////////////////

    // unpack Network Beacon Period field
    uint32_t temp = mac_pdu_offset[1] >> 4;
    network_beacon_period = common::adt::from_coded_value<network_beacon_period_t>(temp);

    // unpack Cluster Beacon Period field
    temp = mac_pdu_offset[1] & common::adt::bitmask_lsb<4>();
    cluster_beacon_period = common::adt::from_coded_value<cluster_beacon_period_t>(temp);

    //////////////////////////////////////////
    // unpack required fields in octet 2 and 3
    //////////////////////////////////////////

    // unpack Next Cluster Channel field
    next_cluster_channel = mac_pdu_offset[3];
    next_cluster_channel |= (mac_pdu_offset[2] & common::adt::bitmask_lsb<5>()) << 8;

    //////////////////////////////////////////
    // unpack required fields in octets 4 to 7
    //////////////////////////////////////////

    // unpack Time To Next field
    time_to_next = common::adt::b2l_lower(&mac_pdu_offset[4], 4);

    ////////////////////////////////////
    // unpack optional fields if present
    ////////////////////////////////////

    uint32_t offset = 8;

    // if included, unpack optional field Clusters Max TX Power
    if (tx_power) {
        temp = mac_pdu_offset[offset] & common::adt::bitmask_lsb<4>();
        clusters_max_tx_power = common::adt::from_coded_value<clusters_max_tx_power_t>(temp);
        ++offset;
    }

    // if included, unpack optional field Current Cluster Channel
    if (current) {
        temp = mac_pdu_offset[offset + 1];
        temp |= (mac_pdu_offset[offset] & common::adt::bitmask_lsb<5>()) << 8;
        current_cluster_channel = temp;
        offset += 2;
    }

    // if included, unpack optional field Additional Network Beacon Channel (0)
    if (include_network_beacon_channel_0) {
        temp = mac_pdu_offset[offset + 1];
        temp |= (mac_pdu_offset[offset] & common::adt::bitmask_lsb<5>()) << 8;
        network_beacon_channel_0 = temp;
        offset += 2;
    }

    // if included, unpack optional field Additional Network Beacon Channel (1)
    if (include_network_beacon_channel_1) {
        temp = mac_pdu_offset[offset + 1];
        temp |= (mac_pdu_offset[offset] & common::adt::bitmask_lsb<5>()) << 8;
        network_beacon_channel_1 = temp;
        offset += 2;
    }

    // if included, unpack optional field Additional Network Beacon Channel (2)
    if (include_network_beacon_channel_2) {
        temp = mac_pdu_offset[offset + 1];
        temp |= (mac_pdu_offset[offset] & common::adt::bitmask_lsb<5>()) << 8;
        network_beacon_channel_2 = temp;
        offset += 2;
    }

    dectnrp_assert(get_packed_size() == offset, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t network_beacon_message_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t length = 8;
    // check whether Clusters Max TX Power field is included
    length += (mac_pdu_offset[0] >> 4) & 1;
    // check whether Current Cluster Channel field is included
    length += ((mac_pdu_offset[0] >> 2) & 1) * 2;
    // check whether Additional Network Beacon Channels are included
    length += (mac_pdu_offset[0] & common::adt::bitmask_lsb<2>()) * 2;
    return length;
}

}  // namespace dectnrp::sp4
