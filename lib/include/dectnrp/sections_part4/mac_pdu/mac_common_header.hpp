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

#include <cstdint>

#include "dectnrp/common/serdes/packing.hpp"

namespace dectnrp::section4 {

class mac_common_header_t : public common::serdes::packing_t {
    public:
        virtual void zero() override = 0;
        virtual bool is_valid() const override = 0;
        virtual uint32_t get_packed_size() const override = 0;
        virtual void pack(uint8_t* mch_front) const override = 0;
        virtual bool unpack(const uint8_t* mch_front) override = 0;
};

class data_mac_pdu_header_t final : public mac_common_header_t {
    public:
        virtual void zero() override final;
        virtual bool is_valid() const override final;
        virtual uint32_t get_packed_size() const override final { return 2; };
        virtual void pack(uint8_t* mch_front) const override final;
        virtual bool unpack(const uint8_t* mch_front) override final;

        uint32_t Reserved;
        uint32_t Reset;
        uint32_t Sequence_number;
};

class beacon_header_t final : public mac_common_header_t {
    public:
        virtual void zero() override final;
        virtual bool is_valid() const override final;
        virtual uint32_t get_packed_size() const override final { return 7; };
        virtual void pack(uint8_t* mch_front) const override final;
        virtual bool unpack(const uint8_t* mch_front) override final;

        void set_Network_ID_3_lsb(const uint32_t Network_ID);

        uint32_t Network_ID_3_lsb;
        uint32_t Transmitter_Address;
};

class unicast_header_t final : public mac_common_header_t {
    public:
        virtual void zero() override final;
        virtual bool is_valid() const override final;
        virtual uint32_t get_packed_size() const override final { return 10; };
        virtual void pack(uint8_t* mch_front) const override final;
        virtual bool unpack(const uint8_t* mch_front) override final;

        uint32_t Reserved;
        uint32_t Reset;
        uint32_t Sequence_number;
        uint32_t Receiver_Address;
        uint32_t Transmitter_Address;
};

class rd_broadcasting_header_t final : public mac_common_header_t {
    public:
        virtual void zero() override final;
        virtual bool is_valid() const override final;
        virtual uint32_t get_packed_size() const override final { return 6; };
        virtual void pack(uint8_t* mch_front) const override final;
        virtual bool unpack(const uint8_t* mch_front) override final;

        uint32_t Reserved;
        uint32_t Reset;
        uint32_t Sequence_number;
        uint32_t Transmitter_Address;
};

class mch_empty_t final : public mac_common_header_t {
    public:
        virtual void zero() override final {};
        virtual bool is_valid() const override final { return true; };
        virtual uint32_t get_packed_size() const override final { return 0; };
        virtual void pack(uint8_t* mch_front) const override final {};
        virtual bool unpack(const uint8_t* mch_front) override final { return true; };
};

}  // namespace dectnrp::section4
