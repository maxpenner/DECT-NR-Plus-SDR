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

#include "dectnrp/sections_part4/mac_messages_and_ie/network_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/rd_capability_ie.hpp"

namespace dectnrp::section4 {

class neighbouring_ie_t final : public mmie_packing_peeking_t {
    public:
        neighbouring_ie_t();

        uint32_t short_rd_id;
        std::optional<rd_capability_ie_t::radio_device_class_t> radio_device_class;
        std::optional<uint32_t> measurement_result_snr;
        std::optional<uint32_t> measurement_result_rssi_2;
        bool has_power_constraints;
        std::optional<uint32_t> next_cluster_channel;
        std::optional<uint32_t> time_to_next;
        network_beacon_message_t::network_beacon_period_t network_beacon_period;
        network_beacon_message_t::cluster_beacon_period_t cluster_beacon_period;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;

        constexpr uint32_t get_packed_size_min_to_peek() const override { return 3; }
        peek_result_t get_packed_size_by_peeking(const uint8_t* mac_pdu_offset) const override;
};

}  // namespace dectnrp::section4
