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

#include "dectnrp/sections_part4/mac_messages_and_ie/cluster_beacon_message.hpp"

#include <cstring>
#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/sections_part2/channel_arrangement.hpp"

namespace dectnrp::sp4 {

cluster_beacon_message_t::cluster_beacon_message_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Cluster_Beacon_Message;

    zero();
    mu = 0;

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void cluster_beacon_message_t::testing_set_random() {
    //

    // dectnrp_assert(is_valid(), "mmie invalid");
}

bool cluster_beacon_message_t::testing_is_equal([[maybe_unused]] const testing_t& rhs) const {
    //

    return true;
}

const char* cluster_beacon_message_t::testing_name() const { return typeid(*this).name(); }

void cluster_beacon_message_t::zero() {
    system_frame_number = common::adt::UNDEFINED_NUMERIC_32;

    clusters_max_tx_power = network_beacon_message_t::clusters_max_tx_power_t::not_defined;
    clusters_max_tx_power.reset();

    has_power_constraints = false;

    frame_offset = common::adt::UNDEFINED_NUMERIC_32;
    frame_offset.reset();

    next_cluster_channel = common::adt::UNDEFINED_NUMERIC_32;
    next_cluster_channel.reset();

    time_to_next = common::adt::UNDEFINED_NUMERIC_32;
    time_to_next.reset();

    network_beacon_period = network_beacon_message_t::network_beacon_period_t::not_defined;
    cluster_beacon_period = network_beacon_message_t::cluster_beacon_period_t::not_defined;

    count_to_trigger = count_to_trigger_t::not_defined;

    rel_quality = quality_threshold_t::not_defined;
    min_quality = quality_threshold_t::not_defined;
}

bool cluster_beacon_message_t::is_valid() const {
    if (system_frame_number > common::adt::bitmask_lsb<8>()) {
        return false;
    }

    if (clusters_max_tx_power.has_value() &&
        !common::adt::is_valid(clusters_max_tx_power.value())) {
        return false;
    }

    if (frame_offset.has_value() &&
        ((mu <= 4 && frame_offset.value() > common::adt::bitmask_lsb<8>()) ||
         (mu > 4 && frame_offset.value() > common::adt::bitmask_lsb<16>()))) {
        return false;
    }

    if (next_cluster_channel.has_value() &&
        !sp2::is_absolute_channel_number_in_range(next_cluster_channel.value())) {
        return false;
    }

    if (!common::adt::is_valid(network_beacon_period)) {
        return false;
    }

    if (!common::adt::is_valid(cluster_beacon_period)) {
        return false;
    }

    if (!common::adt::is_valid(count_to_trigger)) {
        return false;
    }

    if (!common::adt::is_valid(rel_quality)) {
        return false;
    }

    if (!common::adt::is_valid(min_quality)) {
        return false;
    }

    return true;
}

uint32_t cluster_beacon_message_t::get_packed_size() const {
    uint32_t size_packed = 4;
    size_packed += clusters_max_tx_power.has_value();
    size_packed += frame_offset.has_value() * (mu <= 4 ? 1 : 2);
    size_packed += next_cluster_channel.has_value() * 2;
    size_packed += time_to_next.has_value() * 4;
    return size_packed;
}

void cluster_beacon_message_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that beacon message is valid before packing
    dectnrp_assert(is_valid(), "cluster beacon message is not valid");

    /////////////////////////////////
    // set required fields in octet 0
    /////////////////////////////////

    // set SFN field
    mac_pdu_offset[0] = system_frame_number;

    /////////////////////////////////
    // set required fields in octet 1
    /////////////////////////////////

    // set TX Power field
    mac_pdu_offset[1] = clusters_max_tx_power.has_value() << 4;

    // set Power Const field
    mac_pdu_offset[1] |= has_power_constraints << 3;

    // set FO field
    mac_pdu_offset[1] |= frame_offset.has_value() << 2;

    // set Next Channel field
    mac_pdu_offset[1] |= next_cluster_channel.has_value() << 1;

    // set TimeToNext field
    mac_pdu_offset[1] |= time_to_next.has_value();

    /////////////////////////////////
    // set required fields in octet 2
    /////////////////////////////////

    // set Network Beacon Period field
    mac_pdu_offset[2] = std::to_underlying(network_beacon_period) << 4;

    // set Cluster Beacon Period field
    mac_pdu_offset[2] |= std::to_underlying(cluster_beacon_period);

    /////////////////////////////////
    // set required fields in octet 3
    /////////////////////////////////

    // set CountToTrigger field
    mac_pdu_offset[3] = std::to_underlying(count_to_trigger) << 4;

    // set RelQuality field
    mac_pdu_offset[3] |= std::to_underlying(rel_quality) << 2;

    // set MinQuality field
    mac_pdu_offset[3] |= std::to_underlying(min_quality);

    /////////////////////////////////
    // set optional fields if present
    /////////////////////////////////

    uint32_t offset = 4;

    // if included, set optional field Clusters Max TX Power
    if (clusters_max_tx_power.has_value()) {
        mac_pdu_offset[offset] = std::to_underlying(clusters_max_tx_power.value());
        ++offset;
    }

    // if included, set optional field Frame Offset
    if (frame_offset.has_value()) {
        uint32_t fo_size = mu <= 4 ? 1 : 2;
        common::adt::l2b_lower(&mac_pdu_offset[offset], frame_offset.value(), fo_size);
        offset += fo_size;
    }

    // if included, set optional field Next Cluster Channel
    if (next_cluster_channel.has_value()) {
        uint32_t temp = next_cluster_channel.value();
        mac_pdu_offset[offset] = temp >> 8;
        mac_pdu_offset[offset + 1] = temp & common::adt::bitmask_lsb<8>();
        offset += 2;
    }

    // if included, set optional field Time To Next
    if (time_to_next.has_value()) {
        common::adt::l2b_lower(&mac_pdu_offset[offset], time_to_next.value(), 4);
        offset += 4;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == offset,
                   "lengths do not match");
}

bool cluster_beacon_message_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    ////////////////////////////////////
    // unpack required fields in octet 0
    ////////////////////////////////////

    // unpack SFN field
    system_frame_number = mac_pdu_offset[0];

    ////////////////////////////////////
    // unpack required fields in octet 1
    ////////////////////////////////////

    // unpack TX Power field
    bool tx_power = (mac_pdu_offset[1] >> 4) & 1;

    // unpack Power Const field
    has_power_constraints = (mac_pdu_offset[1] >> 3) & 1;

    // unpack FO field
    bool fo = (mac_pdu_offset[1] >> 2) & 1;

    // unpack Next Channel field
    bool next_channel = (mac_pdu_offset[1] >> 1) & 1;

    // unpack TimeToNext field
    bool include_time_to_next = mac_pdu_offset[1] & 1;

    ////////////////////////////////////
    // unpack required fields in octet 2
    ////////////////////////////////////

    // unpack Network Beacon Period field
    uint32_t temp = mac_pdu_offset[2] >> 4;
    network_beacon_period =
        common::adt::from_coded_value<network_beacon_message_t::network_beacon_period_t>(temp);

    // unpack Cluster Beacon Period field
    temp = mac_pdu_offset[2] & common::adt::bitmask_lsb<4>();
    cluster_beacon_period =
        common::adt::from_coded_value<network_beacon_message_t::cluster_beacon_period_t>(temp);

    ////////////////////////////////////
    // unpack required fields in octet 3
    ////////////////////////////////////

    // unpack CountToTrigger field
    temp = mac_pdu_offset[3] >> 4;
    count_to_trigger = common::adt::from_coded_value<count_to_trigger_t>(temp);

    // unpack RelQuality field
    temp = (mac_pdu_offset[3] >> 2) & common::adt::bitmask_lsb<2>();
    rel_quality = common::adt::from_coded_value<quality_threshold_t>(temp);

    // unpack MinQuality field
    temp = mac_pdu_offset[3] & common::adt::bitmask_lsb<2>();
    min_quality = common::adt::from_coded_value<quality_threshold_t>(temp);

    ////////////////////////////////////
    // unpack optional fields if present
    ////////////////////////////////////

    uint32_t offset = 4;

    // if included, unpack optional field Clusters Max TX Power
    if (tx_power) {
        temp = mac_pdu_offset[offset] & common::adt::bitmask_lsb<4>();
        clusters_max_tx_power =
            common::adt::from_coded_value<network_beacon_message_t::clusters_max_tx_power_t>(temp);
        ++offset;
    }

    // if included, unpack optional field Frame Offset
    if (fo) {
        uint32_t fo_size = mu <= 4 ? 1 : 2;
        temp = common::adt::b2l_lower(&mac_pdu_offset[offset], fo_size);
        frame_offset = temp;
        offset += fo_size;
    }

    // if included, unpack optional field Next Cluster Channel
    if (next_channel) {
        temp = mac_pdu_offset[offset + 1];
        temp |= (mac_pdu_offset[offset] & common::adt::bitmask_lsb<5>()) << 8;
        next_cluster_channel = temp;
        offset += 2;
    }

    // if included, unpack optional field Time To Next
    if (include_time_to_next) {
        temp = common::adt::b2l_lower(&mac_pdu_offset[offset], 4);
        time_to_next = temp;
        offset += 4;
    }

    dectnrp_assert(get_packed_size() == offset, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t cluster_beacon_message_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t length = 4;
    // check whether Clusters Max TX Power field is included
    length += (mac_pdu_offset[1] >> 4) & 1;
    // check whether Frame Offset field is included
    length += ((mac_pdu_offset[1] >> 2) & 1) * (mu <= 4 ? 1 : 2);
    // check whether Next Cluster Channel field is included
    length += ((mac_pdu_offset[1] >> 1) & 1) * 2;
    // check whether Time To Next field is included
    length += (mac_pdu_offset[1] & 1) * 4;
    return length;
}

}  // namespace dectnrp::sp4
