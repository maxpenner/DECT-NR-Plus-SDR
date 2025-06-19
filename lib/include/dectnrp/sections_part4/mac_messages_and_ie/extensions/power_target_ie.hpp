/*
 * Copyright 2023-2025 Maxim Penner
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

#include <cmath>

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::sp4::extensions {

class power_target_ie_t final : public mmie_packing_t {
    public:
        [[nodiscard]] power_target_ie_t();

        void set_power_target_dBm_coded(const float power_target_dBm) {
            power_target_dBm_coded = static_cast<decltype(power_target_dBm_coded)>(
                std::abs(std::roundf(power_target_dBm)));
        }

        [[nodiscard]] float get_power_target_dBm_uncoded() const {
            return -static_cast<float>(power_target_dBm_coded);
        }

        /// power target is negative value encoded as positive
        uint32_t power_target_dBm_coded;

    private:
        void zero() override;
        [[nodiscard]] bool is_valid() const override;
        [[nodiscard]] uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        [[nodiscard]] bool unpack(const uint8_t* mac_pdu_offset) override;
};

}  // namespace dectnrp::sp4::extensions
