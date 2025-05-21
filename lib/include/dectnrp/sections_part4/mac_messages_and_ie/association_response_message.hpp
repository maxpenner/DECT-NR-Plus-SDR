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
#include <set>

#include "dectnrp/sections_part4/mac_messages_and_ie/association_request_message.hpp"

namespace dectnrp::sp4 {

class association_response_message_t final : public mmie_packing_peeking_t {
    public:
        // Table 6.4.2.5-2, field Reject Cause
        enum class reject_cause_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            radio_capacity_not_sufficient = 0,
            hw_capacity_not_sufficient,
            conflicting_short_rd_id,
            association_request_not_secure,
            other,
            upper
        };

        // Table 6.4.2.5-2, field Reject Time
        enum class reject_time_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _0_s = 0,
            _5_s,
            _10_s,
            _30_s,
            _60_s,
            _120_s,
            _180_s,
            _300_s,
            _600_s,
            upper
        };

        enum class nof_flows_accepted_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            none = 0,
            as_included,
            as_requested = 0b111,
            upper
        };

        struct reject_info_t {
                reject_cause_t reject_cause;
                reject_time_t reject_time;
        };

        struct group_info_t {
                uint32_t group_id;
                uint32_t resource_tag;
        };

        association_response_message_t();

        std::optional<reject_info_t> reject_info;
        std::optional<association_request_message_t::harq_configuration_tx_rx_t> harq_configuration;
        nof_flows_accepted_t nof_flows_accepted;
        std::set<association_request_message_t::flow_id_t> flow_ids;
        std::optional<group_info_t> group_info;
        bool tx_power;

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
