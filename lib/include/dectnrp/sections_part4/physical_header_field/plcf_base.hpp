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

#include "dectnrp/sections_part4/packing.hpp"

namespace dectnrp::section4 {

class plcf_base_t : public packing_t {
    public:
        virtual void zero() override;
        bool is_valid() const override final { return true; };
        virtual uint32_t get_packed_size() const override = 0;
        virtual void pack(uint8_t* plcf_front) const override;
        virtual bool unpack(const uint8_t* plcf_front) override;

        /// will probably be the first byte for any PLCF ever defined
        uint32_t HeaderFormat;
        uint32_t PacketLengthType;
        uint32_t PacketLength_m1;  // physical packet length minus 1 (m1), Table 6.2.1-1

        /// transmit power is always required for AGC, DFMCS for PDC decoding
        uint32_t TransmitPower;
        uint32_t DFMCS;

        // ##################################################
        // convenience functions

        void set_PacketLength_m1(const int32_t PacketLength);
        uint32_t get_PacketLength() const;

        void set_TransmitPower(const int32_t TransmitPower_dBm);
        int32_t get_TransmitPower_dBm() const;

        /**
         * \brief A PLCF can have neither a field for NumberOfSpatialStreams/N_SS nor
         * DFRedundancyVersion explicitly defined. However, they are then implicitly defined as
         * N_SS=1 and DFRedundancyVersion=0 and must be set so by every PLCF of any type and header
         * format. This way we can use the same base class for any PLCF.
         *
         * \return
         */
        virtual uint32_t get_N_SS() const = 0;
        virtual uint32_t get_DFRedundancyVersion() const = 0;
        virtual uint32_t get_Type() const = 0;
        virtual uint32_t get_HeaderFormat() const = 0;

    protected:
        plcf_base_t() = default;

        /// Table 6.2.1-3a: Transmit Power
        static constexpr std::array<int32_t, 16> tx_power_table = {
            -40, -30, -20, -13, -6, -3, 0, 3, 6, 10, 14, 19, 23, 26, 29, 32};
        static constexpr std::array<uint32_t, 9> N_SS_coded_lut = {0, 0, 1, 0, 2, 0, 0, 0, 3};
        static constexpr std::array<uint32_t, 4> N_SS_coded_lut_rev = {1, 2, 4, 8};
};

}  // namespace dectnrp::section4
