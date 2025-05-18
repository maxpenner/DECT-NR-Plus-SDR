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

#include <algorithm>
#include <cstring>
#include <utility>
#include <variant>

#include "dectnrp/sections_part4/mac_messages_and_ie/mmie_pool_tx.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_common_header.hpp"
#include "dectnrp/sections_part4/mac_pdu/mac_header_type.hpp"

#define ASSERT_MMIE_COUNT_EXACT(dec_name, mmie_ptr_name, x)                                        \
    dectnrp_assert(dec_name.get_N_mmie<std::remove_pointer<decltype(mmie_ptr_name)>::type>() == x, \
                   "incorrect number of MMIEs");

namespace dectnrp::section4 {

class mac_pdu_decoder_t final : private mmie_pool_tx_t {
    public:
        mac_pdu_decoder_t();
        ~mac_pdu_decoder_t() = default;

        mac_pdu_decoder_t(const mac_pdu_decoder_t&) = delete;
        mac_pdu_decoder_t& operator=(const mac_pdu_decoder_t&) = delete;
        mac_pdu_decoder_t(mac_pdu_decoder_t&&) = delete;
        mac_pdu_decoder_t& operator=(mac_pdu_decoder_t&&) = delete;

        // ##################################################
        // MAC PDU decoding

        /**
         * \brief Reset the state machine to its initial state. Must be called before invoking
         * decode().
         *
         * \param a_ pointer to the start of the HARQ buffer
         * \param a_cnt_w_tb_ total number of bytes that a will contain
         * \param mu_ subcarrier scaling factor
         */
        void set_configuration(uint8_t* a_, const uint32_t a_cnt_w_tb_, const uint32_t mu_);

        /**
         * \brief Decodes the MAC PDU and verifies that a_cnt_w pointer does not exceed a_cnt_w_tb
         * that must be set before calling decode().
         *
         * \param a_cnt_w_ current number of processable bytes written to the HARQ buffer
         */
        void decode(const uint32_t a_cnt_w_);

        /**
         * \brief Convenience function to check if internal state machine has reached a valid final
         * state.
         *
         * \return
         * \return
         */
        [[nodiscard]] bool has_reached_valid_final_state() const;

        // ##################################################
        // MAC PDU retrieval after decoding, delivers valid results only if a_cnt_w = a_cnt_w_tb
        // bytes have been provided.

        /**
         * \brief Get raw bytes. This function is useful if we are interested in the raw binary data
         * instead of decoded MAC messages and Information Elements (MMIE).
         *
         * \return
         */
        [[nodiscard]] std::pair<const uint8_t*, const uint32_t> get_a_raw() const {
            return std::make_pair(a, a_cnt_w_tb);
        }

        /// convenience function to copy raw content
        void copy_a(uint8_t* a_dst) const { std::memcpy(a_dst, a, a_cnt_w_tb); }

        /**
         * \brief Convenience function to check if there are is any readable decoded MMIE in the MAC
         * PDU.
         *
         * \return
         * \return
         */
        [[nodiscard]] bool has_any_data() const { return !mmie_decoded_vec.empty(); }

        /**
         * \brief Pointer to extract information from the MAC header type.
         *
         * \return nullptr if decoding failed
         */
        [[nodiscard]] const mac_header_type_t* get_mac_header_type() const { return mht_effective; }

        /**
         * \brief Pointer to extract information from the MAC common header.
         *
         * \return nullptr if decoding failed
         */
        [[nodiscard]] const mac_common_header_t* get_mac_common_header() const {
            return mch_effective;
        }

        /**
         * \brief Vector to base class pointers for decoded MMIEs. The exact type of an MMIE can be
         * deduced with a dynamic_cast.
         *
         * \return empty vector if decoding failed
         */
        [[nodiscard]] const std::vector<section4::mmie_t*>& get_mmie_decoded_vec() const {
            return mmie_decoded_vec;
        }

        /// number of specific MMIE, primarily used for asserts
        template <std::derived_from<section4::mmie_t> T>
        [[nodiscard]] size_t get_N_mmie() const {
            return std::count_if(mmie_decoded_vec.begin(), mmie_decoded_vec.end(), [](auto elem) {
                return dynamic_cast<T*>(elem) != nullptr;
            });
        }

    private:
        /// points to the start of the HARQ buffer
        const uint8_t* a;

        /// total number of bytes to be read from the HARQ buffer a.k.a. the transport block size
        uint32_t a_cnt_w_tb;

        /// number of bytes already read from the HARQ buffer
        uint32_t a_cnt_r;

        enum class STATES {
            MAC_HEADER_TYPE,
            MAC_COMMON_HEADER,
            MAC_MUX_HEADER_UNPACK_MAC_EXT_IE_TYPE,
            A_MAC_MUX_HEADER_UNPACK_LENGTH_OR_FIXED_SIZE,
            B_MAC_MESSAGE_IE_PEEK,
            MAC_MESSAGE_IE_UNPACK,
            MAC_PDU_CHECK_IF_DONE,
            MAC_PDU_DONE,
            MAC_PDU_PREMATURE_ABORT
        } state;

        // ##################################################
        // MAC Header Type

        mac_header_type_t* mht_effective;
        mac_header_type_t mht;

        // ##################################################
        // MAC Common Header

        mac_common_header_t* mch_effective;
        std::variant<data_mac_pdu_header_t,
                     beacon_header_t,
                     unicast_header_t,
                     rd_broadcasting_header_t,
                     mch_empty_t>
            mch_variant;

        // ##################################################
        // MAC Messages and IEs

        /// subcarrier scaling factor, determines the size of some MMIE fields
        uint32_t mu;

        /// number of bytes required to unpack the next MAC multiplexing header or MMIE
        uint32_t N_bytes_required;

        /// working copy of MAC multiplexing header
        mac_multiplexing_header_t mmh;

        /// current MMIE being decoded
        mmie_t* mmie;

        /// mapping between MMIE type and an index that indicates which MMIE instance of that
        /// type to use next when decoding
        std::unordered_map<std::type_index, std::size_t> index_next_ie;

        /// stores raw pointers to already decoded MMIEs
        std::vector<section4::mmie_t*> mmie_decoded_vec;

        /**
         * \brief Gets a message IE from the pool that corresponds to a given type index and has not
         * yet been used for decoding. Returns nullptr in case that a message IE of the requested
         * type is not available.
         *
         * \param type_idx type index that maps to an instance of the requested MMIE type
         * \return MMIE of requested type or nullptr
         */
        [[nodiscard]] mmie_t* get_mmie_from_pool(const std::type_index& type_idx);
};

}  // namespace dectnrp::section4
