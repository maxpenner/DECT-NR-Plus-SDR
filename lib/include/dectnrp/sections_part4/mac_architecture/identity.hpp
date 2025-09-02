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

#include <cstdint>

namespace dectnrp::sp4::mac_architecture {

class identity_t {
    public:
        identity_t() = default;
        explicit identity_t(const uint32_t NetworkID_,
                            const uint32_t LongRadioDeviceID_,
                            const uint32_t ShortRadioDeviceID_);

        static constexpr uint32_t NetworkID_reserved{0};

        static constexpr uint32_t LongRadioDeviceID_reserved{0};
        static constexpr uint32_t LongRadioDeviceID_backend{0xFFFFFFFE};
        static constexpr uint32_t LongRadioDeviceID_broadcast{0xFFFFFFFF};

        static constexpr uint32_t ShortRadioDeviceID_reserved{0};
        static constexpr uint32_t ShortRadioDeviceID_broadcast{0xFFFF};

        static constexpr uint32_t full_to_short_network_id(const uint32_t NetworkID) {
            return NetworkID & 0xff;
        }

        uint32_t NetworkID;       // 32 bits
        uint32_t ShortNetworkID;  // 8 LSB

        uint32_t LongRadioDeviceID;   // 32 bits
        uint32_t ShortRadioDeviceID;  // 16 bits, not related to LongRadioDeviceID

        static bool is_valid_NetworkID(const uint32_t NetworkID);
        static bool is_valid_ShortNetworkID(const uint32_t ShortNetworkID);
        static bool is_valid_LongRadioDeviceID(const uint32_t LongRadioDeviceID);
        static bool is_valid_ShortRadioDeviceID(const uint32_t ShortRadioDeviceID);
};

}  // namespace dectnrp::sp4::mac_architecture
