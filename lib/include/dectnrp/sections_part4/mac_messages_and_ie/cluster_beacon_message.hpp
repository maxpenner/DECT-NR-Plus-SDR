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

#include "dectnrp/common/serdes/testing.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/network_beacon_message.hpp"

namespace dectnrp::sp4 {

class cluster_beacon_message_t final : public mmie_packing_peeking_t,
                                       public mu_depending_t,
                                       public common::serdes::testing_t {
    public:
        /// Table 6.4.2.3-1, field CountToTrigger
        enum class count_to_trigger_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _1_times = 0,
            _2_times,
            _3_times,
            _4_times,
            _5_times,
            _6_times,
            _7_times,
            _8_times,
            _16_times,
            _24_times,
            _32_times,
            _40_times,
            _48_times,
            _64_times,
            _128_times,
            _256_times,
            upper
        };

        /// Table 6.4.2.3-1, fields RelQuality and MinQuality
        enum class quality_threshold_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _0_dB = 0,
            _3_dB,
            _6_dB,
            _9_dB,
            upper
        };

        cluster_beacon_message_t();

        uint32_t system_frame_number;
        std::optional<network_beacon_message_t::clusters_max_tx_power_t> clusters_max_tx_power;
        bool has_power_constraints;
        std::optional<uint32_t> frame_offset;
        std::optional<uint32_t> next_cluster_channel;
        std::optional<uint32_t> time_to_next;
        network_beacon_message_t::network_beacon_period_t network_beacon_period;
        network_beacon_message_t::cluster_beacon_period_t cluster_beacon_period;
        count_to_trigger_t count_to_trigger;
        quality_threshold_t rel_quality;
        quality_threshold_t min_quality;

        void testing_set_random() override final;
        bool testing_is_equal(const testing_t& rhs) const override final;
        const char* testing_name() const override final;

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
