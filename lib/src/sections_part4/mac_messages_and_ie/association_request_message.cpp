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

#include "dectnrp/sections_part4/mac_messages_and_ie/association_request_message.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/sections_part2/channel_arrangement.hpp"

namespace dectnrp::sp4 {

association_request_message_t::association_request_message_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Association_Request_Message;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void association_request_message_t::zero() {
    setup_cause = setup_cause_t::not_defined;
    has_power_constraints = false;

    // exclude optional fields by default
    ft_configuration = {
        .network_beacon_period = network_beacon_message_t::network_beacon_period_t::not_defined,
        .cluster_beacon_period = network_beacon_message_t::cluster_beacon_period_t::not_defined,
        .next_cluster_channel = common::adt::UNDEFINED_NUMERIC_32,
        .time_to_next = common::adt::UNDEFINED_NUMERIC_32};
    ft_configuration.reset();

    // exclude optional fields by default
    current_cluster_channel = common::adt::UNDEFINED_NUMERIC_32;
    current_cluster_channel.reset();

    harq_configuration.tx.N_harq_processes = common::adt::UNDEFINED_NUMERIC_32;
    harq_configuration.tx.max_harq_retransmission_delay =
        max_harq_retransmission_delay_t::not_defined;
    harq_configuration.rx.N_harq_processes = common::adt::UNDEFINED_NUMERIC_32;
    harq_configuration.rx.max_harq_retransmission_delay =
        max_harq_retransmission_delay_t::not_defined;

    flow_ids.clear();
}

bool association_request_message_t::is_valid() const {
    if (!common::adt::is_valid(setup_cause)) {
        return false;
    }

    if (flow_ids.empty() || !std::all_of(flow_ids.begin(), flow_ids.end(), [](const auto& flow_id) {
            return common::adt::is_valid(flow_id);
        })) {
        return false;
    }

    if (ft_configuration.has_value()) {
        const auto& ft_config = ft_configuration.value();

        if (!common::adt::is_valid(ft_config.network_beacon_period)) {
            return false;
        }

        if (!common::adt::is_valid(ft_config.cluster_beacon_period)) {
            return false;
        }

        if (!sp2::is_absolute_channel_number_in_range(ft_config.next_cluster_channel)) {
            return false;
        }
    }

    if (current_cluster_channel.has_value() &&
        !sp2::is_absolute_channel_number_in_range(current_cluster_channel.value())) {
        return false;
    }

    if (harq_configuration.tx.N_harq_processes > 0b111) {
        return false;
    }

    if (!common::adt::is_valid(harq_configuration.tx.max_harq_retransmission_delay)) {
        return false;
    }

    if (harq_configuration.rx.N_harq_processes > 0b111) {
        return false;
    }

    if (!common::adt::is_valid(harq_configuration.rx.max_harq_retransmission_delay)) {
        return false;
    }

    return true;
}

uint32_t association_request_message_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "association request message is not valid");
    uint32_t packed_size = 4 + flow_ids.size();
    packed_size += ft_configuration.has_value() * 7;
    packed_size += current_cluster_channel.has_value() * 2;
    return packed_size;
}

void association_request_message_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that association request message is valid before packing
    dectnrp_assert(is_valid(), "association request message is not valid");

    // set required fields in octet 0
    mac_pdu_offset[0] = std::to_underlying(setup_cause) << 5;
    mac_pdu_offset[0] |= flow_ids.size() << 2;
    mac_pdu_offset[0] |= has_power_constraints << 1;
    mac_pdu_offset[0] |= ft_configuration.has_value();

    // set required fields in octet 1
    mac_pdu_offset[1] = current_cluster_channel.has_value() << 7;

    // set required fields in octet 2
    mac_pdu_offset[2] = harq_configuration.tx.N_harq_processes << 5;
    mac_pdu_offset[2] |= std::to_underlying(harq_configuration.tx.max_harq_retransmission_delay);

    // set required fields in octet 3
    mac_pdu_offset[3] = harq_configuration.rx.N_harq_processes << 5;
    mac_pdu_offset[3] |= std::to_underlying(harq_configuration.rx.max_harq_retransmission_delay);

    uint32_t offset = 4;

    // set required fields in octet 4
    for (const auto& flow_id : flow_ids) {
        mac_pdu_offset[offset] = std::to_underlying(flow_id);
        ++offset;
    }

    // in case of FT mode operation, the network beacon period, cluster beacon period, next cluster
    // channel and time to next fields shall be included
    if (ft_configuration.has_value()) {
        const auto& ft_config = ft_configuration.value();

        // set network/cluster beacon period fields
        mac_pdu_offset[offset] = std::to_underlying(ft_config.network_beacon_period) << 4;
        mac_pdu_offset[offset] |= std::to_underlying(ft_config.cluster_beacon_period);
        ++offset;

        // set next cluster channel field
        mac_pdu_offset[offset] = ft_config.next_cluster_channel >> 8;
        mac_pdu_offset[offset + 1] = ft_config.next_cluster_channel & common::adt::bitmask_lsb<8>();
        offset += 2;

        // set time to next field
        common::adt::l2b_lower(&mac_pdu_offset[offset], ft_config.time_to_next, 4);
        offset += 4;
    }

    // if included, set optional field current cluster channel
    if (current_cluster_channel.has_value()) {
        mac_pdu_offset[offset] = current_cluster_channel.value() >> 8;
        mac_pdu_offset[offset + 1] =
            current_cluster_channel.value() & common::adt::bitmask_lsb<8>();
        offset += 2;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == offset,
                   "lengths do not match");
}

bool association_request_message_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack required fields in octet 0
    setup_cause = common::adt::from_coded_value<setup_cause_t>(mac_pdu_offset[0] >> 5);
    uint32_t N_flows = (mac_pdu_offset[0] >> 2) & common::adt::bitmask_lsb<3>();
    has_power_constraints = mac_pdu_offset[0] & 0b10;
    bool is_operating_in_ft_mode = mac_pdu_offset[0] & 1;

    // TODO return early when N_flows == 0b111 ?

    // unpack required fields in octet 1
    bool current = mac_pdu_offset[1] >> 7;

    auto unpack_harq_config = [](const uint8_t* src, harq_configuration_t& dst) -> void {
        dst.N_harq_processes = *src >> 5;
        dst.max_harq_retransmission_delay =
            common::adt::from_coded_value<max_harq_retransmission_delay_t>(
                *src & common::adt::bitmask_lsb<5>());
    };

    // unpack required fields in octet 2
    unpack_harq_config(mac_pdu_offset + 2, harq_configuration.tx);

    // unpack required fields in octet 3
    unpack_harq_config(mac_pdu_offset + 3, harq_configuration.rx);

    uint32_t offset = 4;

    // unpack required fields starting from octet 4
    for (uint32_t n = 0; n < N_flows; ++n) {
        uint32_t flow_id = mac_pdu_offset[offset] & common::adt::bitmask_lsb<6>();
        flow_ids.insert(common::adt::from_coded_value<flow_id_t>(flow_id));
        ++offset;
    }

    // in case of FT mode operation, the network beacon period, cluster beacon period, next cluster
    // channel and time to next fields are expected to be included
    if (is_operating_in_ft_mode) {
        ft_configuration_t ft_config;

        // unpack optional network/cluster beacon period fields
        ft_config.network_beacon_period =
            common::adt::from_coded_value<network_beacon_message_t::network_beacon_period_t>(
                mac_pdu_offset[offset] & common::adt::bitmask_msb<4>());
        ft_config.cluster_beacon_period =
            common::adt::from_coded_value<network_beacon_message_t::cluster_beacon_period_t>(
                mac_pdu_offset[offset] & common::adt::bitmask_lsb<4>());
        ++offset;

        // unpack optional next cluster channel field
        ft_config.next_cluster_channel = (mac_pdu_offset[offset] & common::adt::bitmask_lsb<5>())
                                         << 8;
        ft_config.next_cluster_channel |= mac_pdu_offset[offset + 1];
        offset += 2;

        // unpack optional time to next field
        ft_config.time_to_next = common::adt::b2l_lower(mac_pdu_offset + offset, 4);
        offset += 4;

        ft_configuration = ft_config;
    }

    // if included, unpack optional current cluster channel field
    if (current) {
        uint32_t temp = (mac_pdu_offset[offset] & common::adt::bitmask_lsb<5>()) << 8;
        temp |= mac_pdu_offset[offset + 1];
        current_cluster_channel = temp;
        offset += 2;
    }

    dectnrp_assert(get_packed_size() == offset, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t association_request_message_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t size = 4;
    // check how many flow IDs are included
    if (uint32_t N_flows = (mac_pdu_offset[0] >> 2) & 0b111; N_flows < 0b111) {
        size += N_flows;
    } else {
        // TODO check in mac_pdu_decoder_t for UNDEFINED_NUMERIC_32 return value
        return peek_errc::NONRESERVED_FIELD_SET_TO_RESERVED;
    }
    // check whether FT mode bit is set
    size += (mac_pdu_offset[0] & 1) * 7;
    // check whether current cluster channel is included
    size += (mac_pdu_offset[1] >> 7) * 2;
    return size;
}

}  // namespace dectnrp::sp4
