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

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::sp4 {

class configuration_request_ie_t final : public mmie_t {
    public:
        [[nodiscard]] configuration_request_ie_t();

        [[nodiscard]] uint32_t get_packed_size_of_sdu() const override final { return 0; };
        [[nodiscard]] uint32_t get_packed_size_of_mmh_sdu() const override final { return 1; };

        void pack_mmh_sdu(uint8_t* mac_pdu_offset) override final {
            mac_multiplexing_header.pack(mac_pdu_offset);
        }
};

}  // namespace dectnrp::sp4
