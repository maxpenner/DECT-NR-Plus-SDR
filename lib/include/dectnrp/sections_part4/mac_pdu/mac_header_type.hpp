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

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/serdes/packing.hpp"

namespace dectnrp::sp4 {

class mac_header_type_t final : public common::serdes::packing_t {
    public:
        enum class version_ec : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            v00 = 0b00
        };

        enum class mac_security_ec : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            macsec_not_used = 0b0,
            macsec_used_no_ie = 0b1,
            macsec_used_with_ie = 0b10,
            reserved = 0b11
        };

        enum class mac_header_type_ec : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            DATA_MAC_PDU = 0b0,
            Beacon = 0b1,
            Unicast = 0b10,
            RD_Broadcasting = 0b11,
            mch_empty = 0b100,
            Escape = 0b1111
        };

        void zero() override final;
        [[nodiscard]] bool is_valid() const override final;
        [[nodiscard]] uint32_t get_packed_size() const override final { return 1; };
        void pack(uint8_t* mac_pdu_front) const override final;
        [[nodiscard]] bool unpack(const uint8_t* mac_pdu_front) override final;

        version_ec Version{version_ec::not_defined};
        mac_security_ec MAC_security{mac_security_ec::not_defined};
        mac_header_type_ec MAC_header_type{mac_header_type_ec::not_defined};

        [[nodiscard]] static inline version_ec get_version_ec(const uint32_t val) {
            return val == 0 ? static_cast<version_ec>(val) : version_ec::not_defined;
        }

        [[nodiscard]] static inline mac_security_ec get_mac_security_ec(const uint32_t val) {
            return val <= 3 ? static_cast<mac_security_ec>(val) : mac_security_ec::not_defined;
        }

        [[nodiscard]] static inline mac_header_type_ec get_mac_header_type_ec(const uint32_t val) {
            // Escape is not valid
            return val <= 4 ? static_cast<mac_header_type_ec>(val)
                            : mac_header_type_ec::not_defined;
        }
};

}  // namespace dectnrp::sp4
