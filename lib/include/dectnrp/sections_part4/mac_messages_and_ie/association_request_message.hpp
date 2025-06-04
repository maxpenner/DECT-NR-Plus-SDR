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

#include "dectnrp/sections_part4/mac_messages_and_ie/network_beacon_message.hpp"

namespace dectnrp::sp4 {

class association_request_message_t final : public mmie_packing_peeking_t {
    public:
        /// Table 6.4.2.4-2
        enum class setup_cause_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            initial = 0,
            new_set_of_flows_requested,
            mobility,
            error_occurred,
            own_operating_channel_changed,
            operating_mode_changed,
            other,
            upper
        };

        /// Table 6.4.2.4-1, fields MAX HARQ RE-TX and MAX HARQ RE-RX
        enum class max_harq_retransmission_delay_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _105_us = 0,
            _200_us,
            _400_us,
            _800_us,
            _1_ms,
            _2_ms,
            _4_ms,
            _6_ms,
            _8_ms,
            _10_ms,
            _20_ms,
            _30_ms,
            _40_ms,
            _50_ms,
            _60_ms,
            _70_ms,
            _80_ms,
            _90_ms,
            _100_ms,
            _120_ms,
            _140_ms,
            _160_ms,
            _180_ms,
            _200_ms,
            _240_ms,
            _280_ms,
            _320_ms,
            _360_ms,
            _400_ms,
            _450_ms,
            _500_ms,
            upper
        };

        /// Table 6.3.4-2
        enum class flow_id_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            lower = 0,
            higher_layer_signalling_flow_1,
            higher_layer_signalling_flow_2,
            user_plane_data_flow_1,
            user_plane_data_flow_2,
            user_plane_data_flow_3,
            user_plane_data_flow_4,
            upper
        };

        struct ft_configuration_t {
                network_beacon_message_t::network_beacon_period_t network_beacon_period;
                network_beacon_message_t::cluster_beacon_period_t cluster_beacon_period;
                uint32_t next_cluster_channel;
                uint32_t time_to_next;
        };

        struct harq_configuration_t {
                uint32_t N_harq_processes;
                max_harq_retransmission_delay_t max_harq_retransmission_delay;
        };

        struct harq_configuration_tx_rx_t {
                harq_configuration_t tx;
                harq_configuration_t rx;
        };

        association_request_message_t();

        setup_cause_t setup_cause;
        std::set<flow_id_t> flow_ids;
        bool has_power_constraints;
        std::optional<ft_configuration_t> ft_configuration;
        std::optional<uint32_t> current_cluster_channel;
        harq_configuration_tx_rx_t harq_configuration;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;

        constexpr uint32_t get_packed_size_min_to_peek() const override { return 2; }
        peek_result_t get_packed_size_by_peeking(const uint8_t* mac_pdu_offset) const override;
};

}  // namespace dectnrp::sp4
