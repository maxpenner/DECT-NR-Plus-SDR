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

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::sp4 {

class resource_allocation_ie_t final : public mmie_packing_peeking_t, public mu_depending_t {
    public:
        /// Table 6.4.3.3-2
        enum class dect_scheduled_resource_failure_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            lower = 1,
            _20_ms,
            _50_ms,
            _100_ms,
            _200_ms,
            _500_ms,
            _1000_ms,
            _1500_ms,
            _3000_ms,
            _4000_ms,
            _5000_ms,
            upper
        };

        enum class length_type_t : bool {
            length_in_subslots,
            length_in_slots
        };

        enum class repeat_type_t : bool {
            repeated_in_following_frames,
            repeated_in_following_subslots
        };

        struct allocation_t {
                uint32_t start_subslot;
                length_type_t length_type;
                uint32_t length;
        };

        struct repeat_info_t {
                repeat_type_t repeat_type;
                bool allow_specific_repeated_resources;
                uint32_t repetition;
                uint32_t validity;
        };

        [[nodiscard]] resource_allocation_ie_t();

        std::optional<allocation_t> allocation_dl;
        std::optional<allocation_t> allocation_ul;
        bool is_additional_allocation;
        std::optional<uint32_t> short_rd_id;
        std::optional<repeat_info_t> repeat_info;
        std::optional<uint32_t> sfn_offset;
        std::optional<uint32_t> channel;
        std::optional<dect_scheduled_resource_failure_t> dect_scheduled_resource_failure;

    private:
        /// Table 6.4.3.3-1, field Allocation Type
        enum class allocation_type_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            release_all_scheduled_resources = 0,
            dl_resources,
            ul_resources,
            dl_ul_resources,
            upper
        };

        /// Table 6.4.3.3-1, field Repeat
        enum class repeat_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            single_allocation = 0,
            repeated_in_following_frames,
            repeated_in_following_subslots,
            repeated_in_following_frames_allow_specific_repeated_resources,
            repeated_in_following_subslots_allow_specific_repeated_resources,
            upper
        };

        void zero() override;
        [[nodiscard]] bool is_valid() const override;
        [[nodiscard]] uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        [[nodiscard]] bool unpack(const uint8_t* mac_pdu_offset) override;

        [[nodiscard]] constexpr uint32_t get_packed_size_min_to_peek() const override { return 2; }
        [[nodiscard]] peek_result_t get_packed_size_by_peeking(
            const uint8_t* mac_pdu_offset) const override;
};

}  // namespace dectnrp::sp4
