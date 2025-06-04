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

namespace dectnrp::sp4 {

class association_release_message_t final : public mmie_packing_t {
    public:
        /// Table 6.4.2.6-1
        enum class release_cause_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            connection_termination = 0,
            mobility,
            long_inactivity,
            incompatible_configuration,
            no_sufficient_hw_memory_resource,
            no_sufficient_radio_resources,
            bad_radio_quality,
            security_error,
            other_error,
            other_reason,
            upper
        };

        association_release_message_t();

        release_cause_t release_cause;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;
};

}  // namespace dectnrp::sp4
