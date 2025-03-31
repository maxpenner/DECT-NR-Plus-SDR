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

#include "dectnrp/sections_part4/physical_header_field/plcf_base.hpp"

namespace dectnrp::section4 {

class plcf_10_t final : public plcf_base_t {
    public:
        void zero() override final;
        // bool is_valid()
        uint32_t get_packed_size() const override final { return 5; }
        void pack(uint8_t* plcf_front) const override final;
        bool unpack(const uint8_t* plcf_front) override final;

        // uint32_t HeaderFormat;
        // uint32_t PacketLengthType;
        // uint32_t PacketLength_m1;
        uint32_t ShortNetworkID;
        uint32_t TransmitterIdentity;
        // uint32_t TransmitPower;
        uint32_t Reserved;
        // uint32_t DFMCS;

        // ##################################################
        // convenience functions

        // Table 6.2.1-1 in part 4 has no field for the nof spatial streams, but is defined as 1
        uint32_t get_N_SS() const override final { return 1; };
        // Table 6.2.1-1 in part 4 has no field for DFRedundancyVersion, but it is defined to be 0
        uint32_t get_DFRedundancyVersion() const override final { return 0; };
        uint32_t get_Type() const override final { return 1; };
        uint32_t get_HeaderFormat() const override final { return 0; };
};

}  // namespace dectnrp::section4
