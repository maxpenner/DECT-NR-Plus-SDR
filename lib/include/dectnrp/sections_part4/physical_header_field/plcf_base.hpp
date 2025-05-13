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

#include <array>
#include <cstdint>
#include <type_traits>

#include "dectnrp/common/serdes/packing.hpp"

namespace dectnrp::section4 {

class plcf_base_t : public common::serdes::packing_t {
    public:
        virtual void zero() override;
        virtual bool is_valid() const override;
        virtual uint32_t get_packed_size() const override = 0;
        virtual void pack(uint8_t* plcf_front) const override;
        virtual bool unpack(const uint8_t* plcf_front) override;

        uint32_t HeaderFormat;
        uint32_t PacketLengthType;
        uint32_t PacketLength_m1;

        uint32_t TransmitPower;
        uint32_t DFMCS;

        void set_PacketLength_m1(const int32_t PacketLength);
        uint32_t get_PacketLength() const;

        void set_TransmitPower(const int32_t TransmitPower_dBm);

        template <typename T>
            requires std::is_signed_v<T>
        T get_TransmitPower_dBm() const {
            return static_cast<T>(tx_power_table.at(TransmitPower));
        }

        virtual uint32_t get_Type() const = 0;
        virtual uint32_t get_HeaderFormat() const = 0;
        virtual uint32_t get_N_SS() const = 0;
        virtual uint32_t get_DFRedundancyVersion() const = 0;

    protected:
        plcf_base_t() = default;

        /// Table 6.2.1-3a: Transmit Power
        static constexpr std::array<int32_t, 16> tx_power_table = {
            -40, -30, -20, -16, -12, -8, -4, 0, 4, 7, 10, 13, 16, 19, 21, 23};

        static constexpr std::array<uint32_t, 9> N_SS_coded_lut = {0, 0, 1, 0, 2, 0, 0, 0, 3};

        static constexpr std::array<uint32_t, 4> N_SS_coded_lut_rev = {1, 2, 4, 8};
};

}  // namespace dectnrp::section4
