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

#include "dectnrp/sections_part4/mac_messages_and_ie/association_response_message.hpp"

#include <utility>

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/common/adt/enumeration.hpp"

namespace dectnrp::sp4 {

association_response_message_t::association_response_message_t() {
    mac_mux_header.zero();
    mac_mux_header.mac_ext = mac_multiplexing_header_t::mac_ext_t::No_Length_Field;
    mac_mux_header.ie_type.mac_ext_00_01_10 =
        mac_multiplexing_header_t::ie_type_mac_ext_00_01_10_t::Association_Response_Message;

    zero();

    dectnrp_assert(has_valid_inheritance_and_properties(this), "mmie invalid");
}

void association_response_message_t::zero() {
    reject_info = {.reject_cause = reject_cause_t::not_defined,
                   .reject_time = reject_time_t::not_defined};
    reject_info.reset();

    association_request_message_t::harq_configuration_tx_rx_t harq_config;

    harq_config.rx.N_harq_processes = common::adt::UNDEFINED_NUMERIC_32;
    harq_config.rx.max_harq_retransmission_delay =
        association_request_message_t::max_harq_retransmission_delay_t::not_defined;

    harq_config.tx.N_harq_processes = common::adt::UNDEFINED_NUMERIC_32;
    harq_config.tx.max_harq_retransmission_delay =
        association_request_message_t::max_harq_retransmission_delay_t::not_defined;

    harq_configuration = harq_config;
    harq_configuration.reset();

    nof_flows_accepted = nof_flows_accepted_t::not_defined;
    flow_ids.clear();

    group_info = {.group_id = common::adt::UNDEFINED_NUMERIC_32,
                  .resource_tag = common::adt::UNDEFINED_NUMERIC_32};
    group_info.reset();

    tx_power = false;
}

bool association_response_message_t::is_valid() const {
    // association rejected
    if (reject_info.has_value()) {
        const auto& reject_info_v = reject_info.value();

        if (!common::adt::is_valid(reject_info_v.reject_cause) ||
            !common::adt::is_valid(reject_info_v.reject_time)) {
            return false;
        }

        return true;
    }

    // association accepted
    if (harq_configuration.has_value()) {
        const auto& harq_config = harq_configuration.value();

        using enum association_request_message_t::max_harq_retransmission_delay_t;

        if (harq_config.rx.N_harq_processes > 0b111 ||
            !common::adt::is_valid(harq_config.rx.max_harq_retransmission_delay) ||
            harq_config.tx.N_harq_processes > 0b111 ||
            !common::adt::is_valid(harq_config.tx.max_harq_retransmission_delay)) {
            return false;
        }
    }

    if (!common::adt::is_valid(nof_flows_accepted) ||
        (nof_flows_accepted == nof_flows_accepted_t::as_included && flow_ids.empty())) {
        return false;
    }

    if (!std::all_of(flow_ids.begin(), flow_ids.end(), [](const auto& flow_id) {
            return common::adt::is_valid(flow_id);
        })) {
        return false;
    }

    if (group_info.has_value()) {
        const auto& group_info_v = group_info.value();

        if (group_info_v.group_id > common::adt::bitmask_lsb<7>() ||
            group_info_v.resource_tag > common::adt::bitmask_lsb<7>()) {
            return false;
        }
    }

    return true;
}

uint32_t association_response_message_t::get_packed_size() const {
    dectnrp_assert(is_valid(), "association response message is not valid");

    // association rejected
    if (reject_info.has_value()) {
        return 2;
    }

    // association accepted
    uint32_t packed_size = 1;
    packed_size += harq_configuration.has_value() * 2;
    packed_size += flow_ids.size();
    packed_size += group_info.has_value() * 2;
    return packed_size;
}

void association_response_message_t::pack(uint8_t* mac_pdu_offset) const {
    // assert that association request message is valid before packing
    dectnrp_assert(is_valid(), "association request message is not valid");

    // if the association is rejected, "the association response includes only the Reject Cause and
    // rejectTime fields in the second octet."
    if (reject_info.has_value()) {
        mac_pdu_offset[0] = 0;
        mac_pdu_offset[1] = std::to_underlying(reject_info.value().reject_cause) << 4;
        mac_pdu_offset[1] |= std::to_underlying(reject_info.value().reject_time);

        dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
        dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == 2,
                       "lengths do not match");
        return;
    }

    // association accepted
    mac_pdu_offset[0] = 1 << 7;
    mac_pdu_offset[0] |= harq_configuration.has_value() << 5;
    mac_pdu_offset[0] |= (nof_flows_accepted == nof_flows_accepted_t::as_included
                              ? flow_ids.size()
                              : std::to_underlying(nof_flows_accepted))
                         << 2;
    mac_pdu_offset[0] |= group_info.has_value() << 1;
    mac_pdu_offset[0] |= tx_power;

    uint32_t offset = 1;

    // set optional HARQ configuration
    if (harq_configuration.has_value()) {
        const auto& harq_config = harq_configuration.value();

        mac_pdu_offset[offset] = harq_config.rx.N_harq_processes << 5;
        mac_pdu_offset[offset] |= std::to_underlying(harq_config.rx.max_harq_retransmission_delay);
        ++offset;

        mac_pdu_offset[offset] = harq_config.tx.N_harq_processes << 5;
        mac_pdu_offset[offset] |= std::to_underlying(harq_config.tx.max_harq_retransmission_delay);
        ++offset;
    }

    // set optional flow ID fields
    for (const auto& flow_id : flow_ids) {
        mac_pdu_offset[offset] = std::to_underlying(flow_id);
        ++offset;
    }

    // set optional fields group ID and resource tag
    if (group_info.has_value()) {
        mac_pdu_offset[offset] = group_info.value().group_id;
        mac_pdu_offset[offset + 1] = group_info.value().resource_tag;
        offset += 2;
    }

    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).has_value(), "peeking failed");
    dectnrp_assert(get_packed_size_by_peeking(mac_pdu_offset).value() == offset,
                   "lengths do not match");
}

bool association_response_message_t::unpack(const uint8_t* mac_pdu_offset) {
    zero();

    // if the association is rejected, "the association response includes only the Reject Cause and
    // rejectTime fields in the second octet."
    if (~mac_pdu_offset[0] >> 7) {
        reject_info = {
            .reject_cause = common::adt::from_coded_value<reject_cause_t>(mac_pdu_offset[1] >> 4),
            .reject_time = common::adt::from_coded_value<reject_time_t>(mac_pdu_offset[1] & 0xf)};
        dectnrp_assert(get_packed_size() == 2, "lengths do not match");
        return is_valid();
    }

    // association accepted

    uint32_t offset = 1;

    // unpack optional HARQ fields
    if ((mac_pdu_offset[0] >> 5) & 1) {
        auto unpack_harq_config =
            [](const uint8_t* src,
               association_request_message_t::harq_configuration_t& dst) -> void {
            dst.N_harq_processes = *src >> 5;
            dst.max_harq_retransmission_delay = common::adt::from_coded_value<
                association_request_message_t::max_harq_retransmission_delay_t>(
                *src & common::adt::bitmask_lsb<5>());
        };

        association_request_message_t::harq_configuration_tx_rx_t harq_config;

        unpack_harq_config(mac_pdu_offset + offset, harq_config.rx);
        ++offset;

        unpack_harq_config(mac_pdu_offset + offset, harq_config.tx);
        ++offset;

        harq_configuration = harq_config;
    }

    // unpack optional flow IDs
    switch (uint32_t N_flows = (mac_pdu_offset[0] >> 2) & 0b111) {
        using enum nof_flows_accepted_t;

        case std::to_underlying(none):
        case std::to_underlying(as_requested):
            nof_flows_accepted = common::adt::from_coded_value<nof_flows_accepted_t>(N_flows);
            break;

        default:
            nof_flows_accepted = as_included;
            for (uint32_t n = 0; n < N_flows; ++n) {
                uint32_t flow_id = mac_pdu_offset[offset] & common::adt::bitmask_lsb<6>();
                flow_ids.insert(
                    common::adt::from_coded_value<association_request_message_t::flow_id_t>(
                        flow_id));
                ++offset;
            }
    }

    // unpack optional group ID and resource tag fields
    if (mac_pdu_offset[0] & 0b10) {
        group_info = {.group_id = mac_pdu_offset[offset] & common::adt::bitmask_lsb<7>(),
                      .resource_tag = mac_pdu_offset[offset + 1] & common::adt::bitmask_lsb<7>()};
        offset += 2;
    }

    // unpack TX power field
    tx_power = mac_pdu_offset[0] & 1;

    dectnrp_assert(get_packed_size() == offset, "lengths do not match");

    return is_valid();
}

mmie_packing_peeking_t::peek_result_t association_response_message_t::get_packed_size_by_peeking(
    const uint8_t* mac_pdu_offset) const {
    // association rejected
    if (~mac_pdu_offset[0] >> 7) {
        return 2;
    }

    uint32_t packed_size = 1;

    // HARQ configuration
    packed_size += ((mac_pdu_offset[0] >> 5) & 1) * 2;

    // number of included flow IDs
    if (uint32_t N_flows = (mac_pdu_offset[0] >> 2) & 0b111;
        N_flows != std::to_underlying(nof_flows_accepted_t::as_requested)) {
        packed_size += N_flows;
    }

    // group field
    packed_size += (mac_pdu_offset[0] & 0b10) * 2;

    return packed_size;
}

};  // namespace dectnrp::sp4
