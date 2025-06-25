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

#include "dectnrp/sections_part4/mac_pdu/mac_pdu_decoder.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/higher_layer_signalling.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/padding_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

namespace dectnrp::sp4 {

mac_pdu_decoder_t::mac_pdu_decoder_t() {
    // the RX pool has to contain some MAC messages/IEs more than once
    set_nof_elements<higher_layer_signalling_t>(limits::max_nof_higher_layer_signalling);
    set_nof_elements<user_plane_data_t>(limits::max_nof_user_plane_data_per_mac_pdu);

    index_next_ie.reserve(get_nof_mmie());
    mmie_decoded_vec.reserve(get_nof_mmie_elements());
}

void mac_pdu_decoder_t::set_configuration(uint8_t* a_,
                                          const uint32_t a_cnt_w_tb_,
                                          const uint32_t mu_) {
    dectnrp_assert(a_ != nullptr, "TB starts at nullptr");
    a = a_;
    dectnrp_assert(a_cnt_w_tb_ > 0, "number of bytes in TB must be larger 0");
    a_cnt_w_tb = a_cnt_w_tb_;

    state = STATES::MAC_HEADER_TYPE;

    a_cnt_r = 0;

    mht_effective = nullptr;
    mht.zero();

    mch_effective = nullptr;

    mu = mu_;
    N_bytes_required = mht.get_packed_size();
    mmh.zero();
    mmie = nullptr;
    for (auto& elem : index_next_ie) {
        elem.second = 0;
    }
    mmie_decoded_vec.clear();

    dectnrp_assert(a_cnt_r + N_bytes_required <= a_cnt_w_tb, "first state requires too many bytes");
}

void mac_pdu_decoder_t::decode(const uint32_t a_cnt_w_) {
    dectnrp_assert(a_cnt_w_ <= a_cnt_w_tb, "invalid write counter");
    dectnrp_assert(a_cnt_r <= a_cnt_w_, "invalid read counter");

    auto prepare_mac_pdu_check_if_done = [&]() -> void {
        state = STATES::MAC_PDU_CHECK_IF_DONE;
        N_bytes_required = 0;
    };

    auto prepare_mac_pdu_premature_abort = [&]() -> void {
        state = STATES::MAC_PDU_PREMATURE_ABORT;
        N_bytes_required = 0;
    };

    while (true) {
        // we should never expect more bytes than the transport block will contain
        if (a_cnt_r + N_bytes_required > a_cnt_w_tb) {
            prepare_mac_pdu_premature_abort();
        } else {
            // number of expected bytes is reasonable, have we already collected it?
            if (a_cnt_w_ - a_cnt_r < N_bytes_required) {
                return;
            }
        }

        switch (state) {
            using enum STATES;

            case MAC_HEADER_TYPE:
                {
                    dectnrp_assert(a_cnt_r == 0, "packets always start with MAC header type");
                    dectnrp_assert(0 < a_cnt_w_, "at least one byte must have been written");

                    if (!mht.unpack(a + a_cnt_r)) {
                        prepare_mac_pdu_premature_abort();
                        break;
                    }

                    mht_effective = &mht;

                    a_cnt_r += N_bytes_required;

                    dectnrp_assert(a_cnt_r == 1, "must be one byte");

                    state = MAC_COMMON_HEADER;

                    switch (mht_effective->MAC_header_type) {
                        using enum mac_header_type_t::mac_header_type_ec;
                        case DATA_MAC_PDU:
                            mch_effective = &mch_variant.emplace<data_mac_pdu_header_t>();
                            break;
                        case Beacon:
                            mch_effective = &mch_variant.emplace<beacon_header_t>();
                            break;
                        case Unicast:
                            mch_effective = &mch_variant.emplace<unicast_header_t>();
                            break;
                        case RD_Broadcasting:
                            mch_effective = &mch_variant.emplace<rd_broadcasting_header_t>();
                            break;
                        case mch_empty:
                            mch_effective = &mch_variant.emplace<mch_empty_t>();
                            break;
                        default:
                            dectnrp_assert_failure("MAC header type decoded to undefined");
                            break;
                    }

                    N_bytes_required = mch_effective->get_packed_size();

                    break;
                }

            case MAC_COMMON_HEADER:
                {
                    if (!mch_effective->unpack(a + a_cnt_r)) {
                        // decoding failed, set pointer back to nullptr
                        mch_effective = nullptr;

                        prepare_mac_pdu_premature_abort();
                        break;
                    }

                    a_cnt_r += N_bytes_required;

                    state = MAC_MUX_HEADER_UNPACK_MAC_EXT_IE_TYPE;

                    N_bytes_required = mac_multiplexing_header_t::packed_size_min_to_peek;

                    break;
                }

            case MAC_MUX_HEADER_UNPACK_MAC_EXT_IE_TYPE:
                {
                    mmh.zero();

                    if (!mmh.unpack_mac_ext_ie_type(a + a_cnt_r)) {
                        prepare_mac_pdu_premature_abort();
                        break;
                    }

                    // a_cnt_r is not incremented here, but instead the MAC multiplexing header's
                    // full packed size is added in MAC_MUX_HEADER_UNPACK

                    /* The padding IE is a special case, because it terminates the MAC PDU as far as
                     * MMIEs are concerned. Quoting 6.4.3.8: "When a receiver detects a MAC
                     * extension field: 01 and IE Type: 000000 or MAC extension field 11 and IE
                     * Type: 00000, the receiver can assume that the rest of the MAC PDU, except the
                     * MIC, is padding."
                     */
                    if (*mmh.tinfo == typeid(padding_ie_t)) {
                        prepare_mac_pdu_premature_abort();
                        break;
                    }

                    // get a pointer to the corresponding MMIE, and by that implicitly check whether
                    // another instance of the MMIE is still available in the pool
                    if ((mmie = get_mmie_from_pool(*mmh.tinfo)) == nullptr) {
                        prepare_mac_pdu_premature_abort();
                        break;
                    }

                    /* Next step is to determine whether the size of the MMIE is self-contained. If
                     * not, we have to unpack the length field contained in the mac multiplexing
                     * header before entering MAC_MESSAGE_IE_UNPACK. Otherwise, we have to determine
                     * the size by peeking at the packed MAC message/IE.
                     */
                    if (mmie_packing_peeking_t* mmie_packing_peeking =
                            dynamic_cast<mmie_packing_peeking_t*>(mmie);
                        mmie_packing_peeking != nullptr) {
                        // make a copy of the MAC multiplexing header
                        mmie->mac_multiplexing_header = mmh;

                        // add full MAC multiplexing header packed size
                        a_cnt_r += N_bytes_required;

                        state = B_MAC_MESSAGE_IE_PEEK;

                        // read enough bytes to peak the MMIE size
                        N_bytes_required = mmie_packing_peeking->get_packed_size_min_to_peek();
                    } else {
                        state = A_MAC_MUX_HEADER_UNPACK_LENGTH_OR_FIXED_SIZE;
                        N_bytes_required = mmh.get_packed_size();
                    }

                    break;
                }

            case A_MAC_MUX_HEADER_UNPACK_LENGTH_OR_FIXED_SIZE:
                {
                    mmh.unpack_length(a + a_cnt_r);

                    // make a copy of the MAC multiplexing header
                    mmie->mac_multiplexing_header = mmh;

                    // add full MAC multiplexing header packed size
                    a_cnt_r += N_bytes_required;

                    state = MAC_MESSAGE_IE_UNPACK;

                    // if mmh contains a length, than this is the number of bytes we need
                    if (mmie->mac_multiplexing_header.length < common::adt::UNDEFINED_NUMERIC_32) {
                        N_bytes_required = mmie->mac_multiplexing_header.length;
                    }
                    // if mmh does not contain a length, the mmie must be of fixed size
                    else {
                        N_bytes_required = mmie->get_packed_size_of_sdu();
                    }

                    break;
                }

            case B_MAC_MESSAGE_IE_PEEK:
                {
                    // set subcarrier scaling factor if applicable
                    if (mu_depending_t* mu_depending = dynamic_cast<mu_depending_t*>(mmie)) {
                        mu_depending->set_mu(mu);
                    }

                    // a_cnt_r is not incremented here, but instead the MMIE's full size is added
                    // in MAC_MESSAGE_IE_UNPACK

                    state = MAC_MESSAGE_IE_UNPACK;

                    if (const auto res =
                            dynamic_cast<mmie_packing_peeking_t*>(mmie)->get_packed_size_by_peeking(
                                a + a_cnt_r)) {
                        N_bytes_required = res.value();
                    } else {
                        // one MMIE is unpeekable, thus we lose trust in the rest of the MAC PDU
                        prepare_mac_pdu_premature_abort();
                    }

                    break;
                }

            case MAC_MESSAGE_IE_UNPACK:
                {
                    bool is_mmie_unpacked_content_valid = true;

                    if (auto packing = dynamic_cast<mmie_packing_t*>(mmie); packing != nullptr) {
                        // unpack must return true, otherwise one of the fields is set incorrectly
                        if (!packing->unpack(a + a_cnt_r)) {
                            is_mmie_unpacked_content_valid = false;
                        }
                    } else {
                        auto flowing = dynamic_cast<mmie_flowing_t*>(mmie);

                        dectnrp_assert(flowing != nullptr, "must be flowing");

                        // not really unpacking, instead we save the pointer from which we can copy
                        flowing->data_ptr = const_cast<uint8_t*>(a + a_cnt_r);
                    }

                    if (is_mmie_unpacked_content_valid) {
                        // increment index of next available MMIE
                        ++index_next_ie[*mmie->mac_multiplexing_header.tinfo];

                        // message IE is valid, add to vector containing decoded IEs
                        mmie_decoded_vec.push_back(mmie);
                    }

                    // add full MMIE packed size
                    a_cnt_r += N_bytes_required;

                    prepare_mac_pdu_check_if_done();

                    break;
                }

            case MAC_PDU_CHECK_IF_DONE:
                {
                    // check if there is more data to read
                    if (a_cnt_r < a_cnt_w_tb) {
                        state = MAC_MUX_HEADER_UNPACK_MAC_EXT_IE_TYPE;
                        N_bytes_required = mac_multiplexing_header_t::packed_size_min_to_peek;
                        break;
                    } else {
                        state = MAC_PDU_DONE;
                        return;
                    }
                }

            case MAC_PDU_DONE:
                {
                    dectnrp_assert_failure("decoding despite MAC PDU being done");
                    break;
                }

            case MAC_PDU_PREMATURE_ABORT:
                {
                    /* Even though we stopped decoding/demultiplexing MMIEs, we still save the
                     * number of bytes written as read to later verify the state machine reached a
                     * valid final state.
                     */
                    a_cnt_r = a_cnt_w_;
                    return;
                }
        }
    }
}

bool mac_pdu_decoder_t::has_reached_valid_final_state() const {
    dectnrp_assert(
        a_cnt_r == a_cnt_w_tb, "not all bytes given to decoder {} {}", a_cnt_r, a_cnt_w_tb);
    dectnrp_assert((state == STATES::MAC_PDU_DONE) or (state == STATES::MAC_PDU_PREMATURE_ABORT),
                   "state machine left in invalid state");

    return (a_cnt_r == a_cnt_w_tb) and
           ((state == STATES::MAC_PDU_DONE) or (state == STATES::MAC_PDU_PREMATURE_ABORT));
};

mmie_t* mac_pdu_decoder_t::get_mmie_from_pool(const std::type_index& type_idx) {
    auto& vec = pool[type_idx];
    std::size_t& i = index_next_ie[type_idx];
    return i < vec.size() ? vec[i].get() : nullptr;
}

}  // namespace dectnrp::sp4
