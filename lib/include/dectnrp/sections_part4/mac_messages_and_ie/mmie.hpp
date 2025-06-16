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
#include "dectnrp/sections_part4/mac_pdu/mac_multiplexing_header.hpp"

namespace dectnrp::sp4 {

class mmie_t {
    public:
        virtual ~mmie_t() = default;

        /**
         * \brief Number of bytes that would be written if the pack_mmh_sdu() function was called.
         *
         * \return Length in bytes.
         */
        virtual uint32_t get_packed_size_of_mmh_sdu() const {
            return mac_mux_header.get_packed_size();
        }

        /**
         * \brief In this base class, only the MAC multiplexing header (mmh) is written in packed
         * form to the destination pointer. Deriving classes pack service data unit (sdu).
         *
         * \param mac_pdu_offset
         */
        virtual void pack_mmh_sdu(uint8_t* mac_pdu_offset) { mac_mux_header.pack(mac_pdu_offset); }

        friend bool has_valid_inheritance_and_properties(const mmie_t* mmie);

        friend class mac_pdu_decoder_t;

    protected:
        mmie_t() = default;
        mac_multiplexing_header_t mac_mux_header;
};

class mmie_packing_t : public mmie_t, public common::serdes::packing_t {
    public:
        uint32_t get_packed_size_of_mmh_sdu() const override final;
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

        virtual constexpr uint32_t get_packed_size_min_to_peek() const = 0;
        virtual peek_result_t get_packed_size_by_peeking(const uint8_t* mac_pdu_offset) const = 0;
};

class mmie_flowing_t : public mmie_t {
    public:
        uint32_t get_packed_size_of_mmh_sdu() const override final;
        void pack_mmh_sdu(uint8_t* mac_pdu_offset) override final;

        /// every IE with a flow ID happens to be a non-packing IE
        virtual void set_flow_id(const uint32_t flow_id) = 0;
        virtual uint32_t get_flow_id() const = 0;

        /// flowing lengths are not self-contained, they have to be defined with these functions
        void set_data_size(const uint32_t N_bytes);
        uint32_t get_data_size() const;

        /// After calling set_flow_id() and set_data_size(), this function returns the destination
        /// address to which at most N_bytes can be copied. Must be done externally.
        uint8_t* get_data_ptr() const;

        friend class mac_pdu_decoder_t;

    protected:
        uint8_t* data_ptr;
};

class mu_depending_t {
    public:
        uint32_t get_mu() const { return mu; }

        void set_mu(const uint32_t mu_) {
            dectnrp_assert(std::has_single_bit(mu_) && mu_ <= 8, "Mu must be 1, 2, 4 or 8");
            mu = mu_;
        }

    protected:
        uint32_t mu;
};

}  // namespace dectnrp::sp4
