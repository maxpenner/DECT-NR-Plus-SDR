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

class group_assignment_ie_t final : public mmie_packing_t {
    public:
        // Table 6.4.3.9-1, field SINGLE
        enum class nof_resource_assignments_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            multiple_assignments = 0,
            single_assignment,
            upper
        };

        // Table 6.4.3.9-1, field DIRECT
        enum class resource_direction_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            as_preassigned = 0,
            inverted,
            upper
        };

        struct resource_assignment_t {
                resource_direction_t direct;
                uint32_t resource_tag;
        };

        group_assignment_ie_t();

        nof_resource_assignments_t single;
        uint32_t group_id;
        std::vector<resource_assignment_t> resource_assignments;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;
};

}  // namespace dectnrp::section4
