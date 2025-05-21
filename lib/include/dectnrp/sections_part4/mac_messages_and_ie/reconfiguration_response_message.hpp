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

#pragma once

#include <optional>

#include "dectnrp/sections_part4/mac_messages_and_ie/association_response_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/reconfiguration_request_message.hpp"

namespace dectnrp::sp4 {

class reconfiguration_response_message_t final : public mmie_packing_peeking_t {
    public:
        reconfiguration_response_message_t();

        std::optional<association_request_message_t::harq_configuration_t> harq_configuration_tx;
        std::optional<association_request_message_t::harq_configuration_t> harq_configuration_rx;
        bool rd_capability_ie_follows;
        association_response_message_t::nof_flows_accepted_t nof_flows_accepted;
        reconfiguration_request_message_t::radio_resource_change_t radio_resource_change;
        std::set<reconfiguration_request_message_t::flow_t> flows;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;

        constexpr uint32_t get_packed_size_min_to_peek() const override { return 1; }
        peek_result_t get_packed_size_by_peeking(const uint8_t* mac_pdu_offset) const override;
};

}  // namespace dectnrp::sp4
