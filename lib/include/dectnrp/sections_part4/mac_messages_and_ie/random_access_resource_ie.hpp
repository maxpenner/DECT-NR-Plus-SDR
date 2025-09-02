/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/sections_part4/mac_messages_and_ie/resource_allocation_ie.hpp"

namespace dectnrp::sp4 {

class random_access_resource_ie_t final : public mmie_packing_peeking_t, public mu_depending_t {
    public:
        enum class cw_min_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _0 = 0,
            _8,
            _16,
            _24,
            _32,
            _40,
            _48,
            _56,
            upper
        };

        enum class dect_delay_t : bool {
            response_window_starts_after_3_subslots,
            response_window_starts_after_half_frame
        };

        enum class cw_max_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _0 = 0,
            _256,
            _512,
            _768,
            _1024,
            _1280,
            _1536,
            _1792,
            upper
        };

        struct repeat_info_t {
                resource_allocation_ie_t::repeat_type_t repeat_type{};
                uint32_t repetition{};
                uint32_t validity{};
        };

        [[nodiscard]] random_access_resource_ie_t();

        std::optional<repeat_info_t> repeat_info;
        std::optional<uint32_t> sfn_offset;
        std::optional<uint32_t> channel;
        std::optional<uint32_t> channel_2;
        resource_allocation_ie_t::allocation_t allocation;
        resource_allocation_ie_t::length_type_t max_rach_length_type;
        uint32_t max_rach_length;
        cw_min_t cw_min;
        dect_delay_t dect_delay;
        uint32_t response_window_length;
        cw_max_t cw_max;

    private:
        /// Table 6.4.3.4-1, Repeat field
        enum class repeat_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            single_allocation = 0,
            repeated_in_following_frames,
            repeated_in_following_subslots,
            upper
        };

        void zero() override;
        [[nodiscard]] bool is_valid() const override;
        [[nodiscard]] uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        [[nodiscard]] bool unpack(const uint8_t* mac_pdu_offset) override;

        [[nodiscard]] constexpr uint32_t get_packed_size_min_to_peek() const override { return 1; }
        [[nodiscard]] peek_result_t get_packed_size_by_peeking(
            const uint8_t* mac_pdu_offset) const override;
};

}  // namespace dectnrp::sp4
