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

#include "dectnrp/sections_part4/mac_messages_and_ie/reconfiguration_request_message.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::sp4 {

reconfiguration_request_message_t::reconfiguration_request_message_t() {
    mac_multiplexing_header.zero();
    mac_multiplexing_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_multiplexing_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Reconfiguration_Request_Message;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void reconfiguration_request_message_t::zero() {
    association_request_message_t::harq_configuration_t harq_config;
    harq_config.N_harq_processes = common::adt::UNDEFINED_NUMERIC_32;
    harq_config.max_harq_retransmission_delay =
        association_request_message_t::max_harq_retransmission_delay_t::not_defined;

    harq_configuration_tx = harq_config;
    harq_configuration_tx.reset();

    harq_configuration_rx = harq_config;
    harq_configuration_rx.reset();

    rd_capability_ie_follows = false;
    flows.clear();
    radio_resource_change = radio_resource_change_t::not_defined;
}

bool reconfiguration_request_message_t::is_valid() const {
    if (const auto& harq_config = harq_configuration_tx.value();
        harq_configuration_tx.has_value() &&
        (harq_config.N_harq_processes > 0b111 ||
         !common::adt::is_valid(harq_config.max_harq_retransmission_delay))) {
        return false;
    }

    if (const auto& harq_config = harq_configuration_rx.value();
        harq_configuration_rx.has_value() &&
        (harq_config.N_harq_processes > 0b111 ||
         !common::adt::is_valid(harq_config.max_harq_retransmission_delay))) {
        return false;
    }

    if (!std::all_of(flows.begin(), flows.end(), [](const auto& flow) {
            return common::adt::is_valid(flow.id);
        })) {
        return false;
    }

    if (!common::adt::is_valid(radio_resource_change)) {
        return false;
    }

    return true;
}

uint32_t reconfiguration_request_message_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "reconfiguration request message is not valid");
    uint32_t packed_size = 1;
    packed_size += harq_configuration_tx.has_value();
    packed_size += harq_configuration_rx.has_value();
    packed_size += flows.size();
    return packed_size;
}

void reconfiguration_request_message_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that reconfiguration request message is valid before packing
    dectnrp_assert(is_valid(), "reconfiguration request message is not valid");

    // set required fields in octet 0
    mac_pdu_offset[0] = harq_configuration_tx.has_value() << 7;
    mac_pdu_offset[0] |= harq_configuration_rx.has_value() << 6;
    mac_pdu_offset[0] |= rd_capability_ie_follows << 5;
    mac_pdu_offset[0] |= flows.size() << 2;
    mac_pdu_offset[0] |= std::to_underlying(radio_resource_change);

    uint32_t offset = 1;

    // set optional HARQ TX configuration fields
    if (harq_configuration_tx.has_value()) {
        const auto& harq_config = harq_configuration_tx.value();
        mac_pdu_offset[offset] = harq_config.N_harq_processes << 5;
        mac_pdu_offset[offset] |= std::to_underlying(harq_config.max_harq_retransmission_delay);
        ++offset;
    }

    // set optional HARQ RX configuration fields
    if (harq_configuration_rx.has_value()) {
        const auto& harq_config = harq_configuration_rx.value();
        mac_pdu_offset[offset] = harq_config.N_harq_processes << 5;
        mac_pdu_offset[offset] |= std::to_underlying(harq_config.max_harq_retransmission_delay);
        ++offset;
    }

    // set optional setup/release flow ID fields
    for (const auto& flow : flows) {
        mac_pdu_offset[offset] = flow.is_released << 7;
        mac_pdu_offset[offset] |= std::to_underlying(flow.id);
        ++offset;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == offset,
                   "lengths do not match");
}

bool reconfiguration_request_message_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // unpack required fields in octet 0
    bool tx_harq = mac_pdu_offset[0] >> 7;
    bool rx_harq = (mac_pdu_offset[0] >> 6) & 1;
    rd_capability_ie_follows = (mac_pdu_offset[0] >> 5) & 1;
    uint32_t N_flows = (mac_pdu_offset[0] >> 2) & common::adt::bitmask_lsb<3>();
    radio_resource_change = common::adt::from_coded_value<radio_resource_change_t>(
        mac_pdu_offset[0] & common::adt::bitmask_lsb<2>());

    uint32_t offset = 1;

    auto unpack_harq_config = [](const uint8_t* src,
                                 association_request_message_t::harq_configuration_t& dst) -> void {
        dst.N_harq_processes = *src >> 5;
        dst.max_harq_retransmission_delay = common::adt::from_coded_value<
            association_request_message_t::max_harq_retransmission_delay_t>(
            *src & common::adt::bitmask_lsb<5>());
    };

    // unpack optional HARQ TX configuration fields
    if (tx_harq) {
        association_request_message_t::harq_configuration_t harq_config;
        unpack_harq_config(mac_pdu_offset + offset, harq_config);
        harq_configuration_tx = harq_config;
        ++offset;
    }

    // unpack optional HARQ RX configuration fields
    if (rx_harq) {
        association_request_message_t::harq_configuration_t harq_config;
        unpack_harq_config(mac_pdu_offset + offset, harq_config);
        harq_configuration_rx = harq_config;
        ++offset;
    }

    // unpack optional setup/release flow ID fields
    for (uint32_t n = 0; n < N_flows; ++n) {
        bool is_released = mac_pdu_offset[offset] >> 7;
        auto flow_id = common::adt::from_coded_value<association_request_message_t::flow_id_t>(
            mac_pdu_offset[offset] & common::adt::bitmask_lsb<6>());
        flows.insert({.id = flow_id, .is_released = is_released});
        ++offset;
    }

    dectnrp_assert(get_packed_size() == offset, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t reconfiguration_request_message_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    uint32_t packed_size = 1;
    // check whether TX HARQ configuration is included
    packed_size += mac_pdu_offset[0] >> 7;
    // check whether RX HARQ configuration is included
    packed_size += (mac_pdu_offset[0] >> 6) & 1;
    // check the number of flow IDs that are included
    if (uint32_t N_flows = (mac_pdu_offset[0] >> 2) & 0b111; N_flows < 0b111) {
        packed_size += N_flows;
    } else {
        // TODO check in mac_pdu_decoder_t for UNDEFINED_NUMERIC_32 return value
        return peek_errc::NONRESERVED_FIELD_SET_TO_RESERVED;
    }
    return packed_size;
}

}  // namespace dectnrp::sp4
