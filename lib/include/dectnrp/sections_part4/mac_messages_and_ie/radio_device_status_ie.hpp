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

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::section4 {

class radio_device_status_ie_t final : public mmie_packing_t {
    public:
        // Table 6.4.3.13-1, field STATUS FLAG
        enum class status_flag_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            lower = 0,
            memory_full,
            normal_operation,
            upper
        };

        // Table 6.4.3.13-1, field DURATION
        enum class duration_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            _50_ms = 0,
            _100_ms,
            _200_ms,
            _400_ms,
            _600_ms,
            _800_ms,
            _1000_ms,
            _1500_ms,
            _2000_ms,
            _3000_ms,
            _4000_ms,
            unknown,
            upper
        };

        radio_device_status_ie_t();

        status_flag_t status_flag;
        duration_t duration;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;
};

}  // namespace dectnrp::section4
