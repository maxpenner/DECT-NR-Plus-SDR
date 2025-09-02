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

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::sp4 {

class load_info_ie_t final : public mmie_packing_peeking_t {
    public:
        /// Table 6.4.3.10-1, field MAX ASSOC
        enum class max_assoc_size_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _8_bit = 0,
            _16_bit,
            upper
        };

        struct channel_load_t {
                uint32_t percentage_subslots_free;
                uint32_t percentage_subslots_busy;
        };

        [[nodiscard]] load_info_ie_t();

        max_assoc_size_t max_assoc;
        std::optional<uint32_t> rd_pt_load_percentage;
        std::optional<uint32_t> rach_load_percentage;
        std::optional<channel_load_t> channel_load;
        uint32_t traffic_load_percentage;
        uint32_t max_nof_associated_rd;
        uint32_t rd_ft_load_percentage;

    private:
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
