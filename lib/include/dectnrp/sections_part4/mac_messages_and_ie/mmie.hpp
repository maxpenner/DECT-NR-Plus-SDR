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

#include <cstdint>

#include "dectnrp/common/adt/result.hpp"
#include "dectnrp/common/serdes/packing.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_multiplexing_header.hpp"

namespace dectnrp::sp4 {

class mmie_t {
    public:
        virtual ~mmie_t() = default;

        /// sdu = MAC service data unit
        [[nodiscard]] virtual uint32_t get_packed_size_of_sdu() const = 0;

        /// mmh = MAC multiplexing header
        [[nodiscard]] virtual uint32_t get_packed_size_of_mmh_sdu() const = 0;

        virtual void pack_mmh_sdu(uint8_t* mac_pdu_offset) = 0;

        [[nodiscard]] bool is_mmh_sdu_fitting(const uint32_t a_cnt_w,
                                              const uint32_t N_TB_byte) const {
            return a_cnt_w + get_packed_size_of_mmh_sdu() <= N_TB_byte;
        }

        [[nodiscard]] bool is_mmh_sdu_fitting(const uint32_t a_cnt_w,
                                              const sp3::packet_sizes_t& packet_sizes) const {
            return is_mmh_sdu_fitting(a_cnt_w, packet_sizes.N_TB_byte);
        }

        friend bool has_valid_inheritance_and_properties(const mmie_t* mmie);

        friend class mac_pdu_decoder_t;

    protected:
        mmie_t() = default;
        mac_multiplexing_header_t mac_multiplexing_header;
};

class mmie_packing_t : public mmie_t, public common::serdes::packing_t {
    public:
        [[nodiscard]] uint32_t get_packed_size_of_sdu() const override final;
        [[nodiscard]] uint32_t get_packed_size_of_mmh_sdu() const override final;
        void pack_mmh_sdu(uint8_t* mac_pdu_offset) override final;

        friend class mac_pdu_decoder_t;
};

class mmie_packing_peeking_t : public mmie_packing_t {
    public:
        virtual ~mmie_packing_peeking_t() = default;

        enum class [[nodiscard]] peek_errc {
            NONRESERVED_FIELD_SET_TO_RESERVED = 0,
            NONRESERVED_FIELD_SET_TO_UNSUPPORTED,
            RESERVED_FIELD_NOT_ZERO
        };

        using peek_result_t = common::adt::res_t<uint32_t, peek_errc>;

        [[nodiscard]] virtual constexpr uint32_t get_packed_size_min_to_peek() const = 0;
        [[nodiscard]] virtual peek_result_t get_packed_size_by_peeking(
            const uint8_t* mac_pdu_offset) const = 0;
};

class mmie_flowing_t : public mmie_t {
    public:
        [[nodiscard]] uint32_t get_packed_size_of_sdu() const override final;
        [[nodiscard]] uint32_t get_packed_size_of_mmh_sdu() const override final;
        void pack_mmh_sdu(uint8_t* mac_pdu_offset) override final;

        virtual void set_flow_id(const uint32_t flow_id) = 0;
        [[nodiscard]] virtual uint32_t get_flow_id() const = 0;

        /// flowing lengths are not self-contained, they have to be defined with these functions
        void set_data_size(const uint32_t N_bytes);
        [[nodiscard]] uint32_t get_data_size() const;

        /**
         * \brief  After calling set_flow_id() and set_data_size(), this function returns the
         * destination address to which at most N_bytes bytes can be copied. The copy process must
         * be done externally.
         *
         * \return
         */
        [[nodiscard]] uint8_t* get_data_ptr() const;

        friend class mac_pdu_decoder_t;

    protected:
        uint8_t* data_ptr;
};

class mu_depending_t {
    public:
        [[nodiscard]] uint32_t get_mu() const { return mu; }

        void set_mu(const uint32_t mu_);

    protected:
        uint32_t mu;
};

}  // namespace dectnrp::sp4
