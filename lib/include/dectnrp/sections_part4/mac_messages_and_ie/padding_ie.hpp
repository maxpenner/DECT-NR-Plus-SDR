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

#include "dectnrp/common/adt/bitbyte.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::sp4 {

class padding_ie_t final : public mmie_t {
    public:
        [[nodiscard]] padding_ie_t();

        /**
         * \brief Sets the entire length of the padding IE including the MAC multiplexing header.
         *
         * \param N_bytes The number of padding bytes by which to fill the MAC PDU. At maximum, a
         * MAC PDU can be padded by 257 bytes.
         */
        void set_nof_padding_bytes(const uint32_t N_bytes);

        /**
         * \brief In case of a medium MAC SDU (prefixed by a MAC multiplexing header with 8 bit
         * length field), the multiplexing header has a length of 2 bytes and is followed by an SDU
         * with a length of up to 255 bytes. Therefore, a MAC PDU can be padded by no more than 257
         * bytes. For a description of how padding IEs are assembled refer to clause 6.4.3.8 of the
         * TS.
         */
        static constexpr uint32_t N_PADDING_BYTES_MAX{common::adt::bitmask_lsb<8>() + 2};

        [[nodiscard]] uint32_t get_packed_size_of_mmh_sdu() const override final;
        void pack_mmh_sdu(uint8_t* mac_pdu_offset) override final;
};

}  // namespace dectnrp::sp4
