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

class mac_security_info_ie_t final : public mmie_packing_t {
    public:
        // Table 6.4.3.1-1
        enum class version_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            mode_1 = 0,
            upper
        };

        // Table 6.4.3.1-2
        enum class security_iv_type_mode_1_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            one_time_hpc = 0,
            resynchronizing_hpc,
            one_time_hpc_with_hpc_request,
            upper
        };

        union security_iv_type_t {
                security_iv_type_mode_1_t mode_1;
        };

        mac_security_info_ie_t();

        static constexpr version_t version{version_t::mode_1};
        uint32_t key_index;
        security_iv_type_t security_iv_type;
        uint32_t hpc;

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;
};

}  // namespace dectnrp::sp4
