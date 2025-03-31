/*
 * Copyright 2023-2025 Maxim Penner, Mattes Wassmann
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

#include "dectnrp/sections_part4/physical_header_field/feedback_info.hpp"
#include "dectnrp/sections_part4/physical_header_field/plcf_base.hpp"

namespace dectnrp::section4::extension {

class plcf_22_t final : public plcf_base_t {
    public:
        void zero() override;
        // bool is_valid()
        uint32_t get_packed_size() const override { return 10; }
        void pack(uint8_t* plcf_front) const override;
        bool unpack(const uint8_t* plcf_front) override;

        // uint32_t HeaderFormat;
        // uint32_t PacketLengthType;
        // uint32_t PacketLength_m1;
        uint32_t TransmitterIdentity;
        // uint32_t TransmitPower;
        // uint32_t DFMCS;

        /// is this a downlink packet?
        bool dl{true};

        /// packet will be forwarded to higher layers if a CRC error occurs
        bool forward_despite_crc_error{true};

        /// indicator for the length of the STF of the next scheduled packet
        enum class next_scheduled_packet_stf_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            full = 0,
            shortened_to_one_ofdm_symbol,
            none,
            upper
        } next_scheduled_packet_stf;

        /// indicator if the next scheduled packet contains a PLCF
        bool next_scheduled_packet_has_plcf{true};

        uint32_t NumberOfSpatialStreams;

        uint32_t FeedbackFormat;
        mutable uint32_t FeedbackInfo;
        mutable section4::feedback_info_pool_t feedback_info_pool;

        // ##################################################
        // convenience functions

        void set_NumberOfSpatialStreams(const uint32_t N_SS);
        uint32_t get_N_SS() const override;
        uint32_t get_DFRedundancyVersion() const override;
        uint32_t get_Type() const override { return 2; }
        uint32_t get_HeaderFormat() const override { return 2; }
};

}  // namespace dectnrp::section4::extension
