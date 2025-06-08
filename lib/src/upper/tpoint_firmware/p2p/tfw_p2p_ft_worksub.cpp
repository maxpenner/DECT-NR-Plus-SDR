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

#include "dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_ft.hpp"
//

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/cluster_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions//time_announce_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

namespace dectnrp::upper::tfw::p2p {

std::optional<phy::maclow_phy_t> tfw_p2p_ft_t::worksub_pcc_10(
    [[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return std::nullopt;
}

phy::maclow_phy_t tfw_p2p_ft_t::worksub_pcc_20(
    [[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::maclow_phy_t tfw_p2p_ft_t::worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) {
    // cast guaranteed to work
    const auto* plcf_21 =
        static_cast<const sp4::plcf_21_t*>(phy_maclow.pcc_report.plcf_decoder.get_plcf_base(2));

    dectnrp_assert(plcf_21 != nullptr, "cast ill-formed");

    // is this a packet from the correct network, from a known PT, and for this FT?
    if (plcf_21->ShortNetworkID != identity_ft.ShortNetworkID ||
        !contact_list.is_srdid_known(plcf_21->TransmitterIdentity) ||
        plcf_21->ReceiverIdentity != identity_ft.ShortRadioDeviceID) {
        return phy::maclow_phy_t();
    }

    // load long radio device ID of sending PT
    const auto lrdid = contact_list.get_lrdid_from_srdid(plcf_21->TransmitterIdentity);

    // load contact information of PT
    auto& contact = contact_list.get_contact(lrdid);

    contact.sync_report = phy_maclow.sync_report;

    contact.mimo_csi.update_from_feedback(
        plcf_21->FeedbackFormat, plcf_21->feedback_info_pool, phy_maclow.sync_report);

    return worksub_pcc2pdc(phy_maclow,
                           2,
                           identity_ft.NetworkID,
                           0,
                           phy::harq::finalize_rx_t::reset_and_terminate,
                           phy::maclow_phy_handle_t(phy::handle_pcc2pdc_t::th21, lrdid));
}

phy::machigh_phy_t tfw_p2p_ft_t::worksub_pdc_10(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_p2p_ft_t::worksub_pdc_20(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_p2p_ft_t::worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) {
    dectnrp_assert(phy_machigh.pdc_report.crc_status,
                   "worksub should only be called with valid CRC");

    // long radio device ID used as key
    const auto lrdid = phy_machigh.maclow_phy.get_handle_lrdid();

    auto& contact = contact_list.get_contact(lrdid);

    // request vector of base pointers to MMIEs
    const auto& mmie_decoded_vec = phy_machigh.pdc_report.mac_pdu_decoder.get_mmie_decoded_vec();

    // count datagrams to be forwarded
    uint32_t datagram_cnt = 0;

    for (auto mmie : mmie_decoded_vec) {
        if (dynamic_cast<sp4::user_plane_data_t*>(mmie) == nullptr) {
            dectnrp_log_wrn("MMIE not user plane data");
            continue;
        }

        const sp4::user_plane_data_t* upd = static_cast<sp4::user_plane_data_t*>(mmie);

        if (application_client->write_nto(
                contact.conn_idx_client, upd->get_data_ptr(), upd->get_data_size()) > 0) {
            ++datagram_cnt;
        }
    }

    application_client->trigger_forward_nto(datagram_cnt);

    contact.mimo_csi.update_from_phy(
        cqi_lut.get_highest_mcs_possible(phy_machigh.pdc_report.snr_dB),
        phy_machigh.phy_maclow.sync_report);

    return phy::machigh_phy_t();
}

bool tfw_p2p_ft_t::worksub_tx_beacon(phy::machigh_phy_t& machigh_phy) {
    // OPTIONAL: change MIMO mode and dimensions of PLCF and MAC PDU (psdef)
    // -

    // request harq process
    auto* hp_tx = hpp->get_process_tx(ppmp_beacon.plcf_base_effective->get_Type(),
                                      identity_ft.NetworkID,
                                      ppmp_beacon.psdef,
                                      phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable.");
        return false;
    }

    // this is now a well-defined packet size
    const sp3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    // OPTIONAL: change content of PLCF, MAC header type and MAC common header
    // -

    uint32_t a_cnt_w = ppmp_beacon.pack_first_3_header(hp_tx->get_a_plcf(), hp_tx->get_a_tb());

    // change content of cluster_beacon_message_t
    auto& cbm = mmie_pool_tx.get<sp4::cluster_beacon_message_t>();
    cbm.pack_mmh_sdu(hp_tx->get_a_tb() + a_cnt_w);
    a_cnt_w += cbm.get_packed_size_of_mmh_sdu();

    // one time_announce_ie_t per second
    if (stats.beacon_cnt % allocation_ft.get_N_beacons_per_second() == 0) {
        // set values in time_announce_ie_t
        auto& taie = mmie_pool_tx.get<sp4::extensions::time_announce_ie_t>();
        taie.set_time(sp4::extensions::time_announce_ie_t::time_type_t::LOCAL, 0, 0);
        taie.pack_mmh_sdu(hp_tx->get_a_tb() + a_cnt_w);
        a_cnt_w += taie.get_packed_size_of_mmh_sdu();
    }

    // fill up with padding IEs
    mmie_pool_tx.fill_with_padding_ies(hp_tx->get_a_tb() + a_cnt_w,
                                       packet_sizes.N_TB_byte - a_cnt_w);

    uint32_t codebook_index = 0;

    const phy::tx_meta_t& tx_meta = {.optimal_scaling_DAC = false,
                                     .DAC_scale = agc_tx.get_ofdm_amplitude_factor(),
                                     .iq_phase_rad = 0.0f,
                                     .iq_phase_increment_s2s_post_resampling_rad = 0.0f,
                                     .GI_percentage = 5};

    radio::buffer_tx_meta_t buffer_tx_meta = {
        .tx_order_id = tx_order_id, .tx_time_64 = allocation_ft.get_beacon_time_scheduled()};

    ++tx_order_id;
    tx_earliest_64 = allocation_ft.get_beacon_time_scheduled();

    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));

    // set tranmission time of next beacon
    allocation_ft.set_beacon_time_next();

    ++stats.beacon_cnt;

    return true;
}

void tfw_p2p_ft_t::worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) {
    // number of definable packets is limited
    for (uint32_t i = 0; i < max_simultaneous_tx_unicast; ++i) {
        // go over the connection indexes which represent different devices
        for (auto& contact : contact_list.get_contacts_vec()) {
            contact.allocation_pt.set_beacon_time_last_known(
                allocation_ft.get_beacon_time_transmitted());

            const auto tx_opportunity =
                contact.allocation_pt.get_tx_opportunity(mac::allocation::direction_t::downlink,
                                                         buffer_rx.get_rx_time_passed(),
                                                         tx_earliest_64);

            // if no opportunity found, leave machigh_phy as is
            if (tx_opportunity.tx_time_64 < 0) {
                return;
            }

            // change content of headers
            ppmp_unicast.plcf_21.ReceiverIdentity = contact.identity.ShortRadioDeviceID;
            ppmp_unicast.unicast_header.Receiver_Address = contact.identity.LongRadioDeviceID;

            // change feedback info in PLCF
            ppmp_unicast.plcf_21.FeedbackFormat = sp4::feedback_info_t::No_feedback;

            // try to send a packet, may return false if no data of HARQ processes are available
            if (!worksub_tx_unicast(machigh_phy, contact, tx_opportunity)) {
                // break;
            }
        }
    }
}

void tfw_p2p_ft_t::worksub_callback_log(const int64_t now_64) const {
    std::string str = "id=" + std::to_string(id) + " ";

    str += stats.get_as_string();

    str += "tx_power_ant_0dBFS=" + std::to_string(agc_tx.get_power_ant_0dBFS(now_64)) + " ";
    str += "rx_power_ant_0dBFS=" + agc_rx.get_power_ant_0dBFS(now_64).get_readable_list() + " ";

    for (const auto& contact : contact_list.get_contacts_vec()) {
        str += "rx_rms=[";
        str += std::to_string(contact.identity.LongRadioDeviceID) + "]" +
               contact.sync_report.rms_array.get_readable_list() + " ";
    }

    dectnrp_log_inf("{}", str);
}

}  // namespace dectnrp::upper::tfw::p2p
