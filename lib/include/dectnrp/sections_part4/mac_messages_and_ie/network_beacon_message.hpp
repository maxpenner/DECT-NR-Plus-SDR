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

#include <limits>
#include <optional>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::section4 {

class network_beacon_message_t final : public mmie_packing_peeking_t {
    public:
        // Table 6.4.2.2-1, field Clusters Max TX Power
        enum class clusters_max_tx_power_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            lower = 2,
            neg_13_dBm,
            neg_6_dBm,
            neg_3_dBm,
            _0_dBm,
            _3_dBm,
            _6_dBm,
            _10_dBm,
            _14_dBm,
            _19_dBm,
            _23_dBm,
            _26_dBm,
            _29_dBm,
            gt_32_dBm,
            upper
        };

        // Table 6.4.2.2-1, field Network Beacon Period
        enum class network_beacon_period_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _50_ms = 0,
            _100_ms,
            _500_ms,
            _1000_ms,
            _1500_ms,
            _2000_ms,
            _4000_ms,
            upper
        };

        // Table 6.4.2.2-1, field Cluster Beacon Period
        enum class cluster_beacon_period_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _10_ms = 0,
            _50_ms,
            _100_ms,
            _500_ms,
            _1000_ms,
            _1500_ms,
            _2000_ms,
            _4000_ms,
            _8000_ms,
            _16000_ms,
            _32000_ms,
            upper
        };

        /**
         * \brief
         *
         * \param tx_power_dBm
         * \return
         */
        static clusters_max_tx_power_t clusters_max_tx_power_from_dBm(const int32_t tx_power_dBm);

        /**
         * \brief
         *
         * \param tx_power_coded
         * \return
         */
        static int32_t clusters_max_tx_power_to_dBm(const clusters_max_tx_power_t tx_power_coded);

        network_beacon_message_t();

        std::optional<clusters_max_tx_power_t> clusters_max_tx_power;
        bool has_power_constraints;
        std::optional<uint32_t> current_cluster_channel;
        std::optional<uint32_t> network_beacon_channel_0;
        std::optional<uint32_t> network_beacon_channel_1;
        std::optional<uint32_t> network_beacon_channel_2;
        network_beacon_period_t network_beacon_period;
        cluster_beacon_period_t cluster_beacon_period;
        uint32_t next_cluster_channel;
        uint32_t time_to_next;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;

        constexpr uint32_t get_packed_size_min_to_peek() const override { return 1; }
        peek_result_t get_packed_size_by_peeking(const uint8_t* mac_pdu_offset) const override;
};

}  // namespace dectnrp::section4
