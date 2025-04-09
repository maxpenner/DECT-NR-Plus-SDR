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

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
#include "dectnrp/application/vnic/vnic_client.hpp"
#include "dectnrp/application/vnic/vnic_server.hpp"
#else
#include "dectnrp/application/socket/socket_client.hpp"
#include "dectnrp/application/socket/socket_server.hpp"
#endif

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/radio/hw_simulator.hpp"

namespace dectnrp::upper::tfw::p2p {

const std::string tfw_p2p_ft_t::firmware_name("p2p_ft");

tfw_p2p_ft_t::tfw_p2p_ft_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tfw_p2p_base_t(tpoint_config_, mac_lower_) {
#ifdef TFW_P2P_MIMO
    dectnrp_assert(
        1 < buffer_rx.nof_antennas,
        "MIMO requires that FT is able to transmit at least two transmit streams. Change to a radio device class with N_TX larger one.");
#endif

    // ##################################################
    // Radio Layer + PHY

    init_radio();

    if (hw_simulator != nullptr) {
        init_simulation_if_detected();
    }

    // ##################################################
    // MAC Layer

    contact_list.reserve(10);

    // init contact list
    for (uint32_t firmware_id_pt = 0; firmware_id_pt < N_pt; ++firmware_id_pt) {
        // load identity of one PT
        const auto identity_pt = init_identity_pt(firmware_id_pt);

        // add PT as new contact
        contact_list.add_new_contact(identity_pt.LongRadioDeviceID,
                                     identity_pt.ShortRadioDeviceID,
                                     firmware_id_pt,
                                     firmware_id_pt);

        auto& contact = contact_list.get_contact(identity_pt.LongRadioDeviceID);

        contact.identity = identity_pt;
        contact.allocation_pt = init_allocation_pt(firmware_id_pt);
        contact.sync_report = phy::sync_report_t(buffer_rx.nof_antennas);
        contact.mimo_csi = phy::mimo_csi_t();

        dectnrp_assert(contact.identity.NetworkID == identity_pt.NetworkID, "id not equal");
        dectnrp_assert(contact.identity.ShortNetworkID == identity_pt.ShortNetworkID,
                       "id not equal");
        dectnrp_assert(contact.identity.LongRadioDeviceID == identity_pt.LongRadioDeviceID,
                       "id not equal");
        dectnrp_assert(contact.identity.ShortRadioDeviceID == identity_pt.ShortRadioDeviceID,
                       "id not equal");
    }

    init_packet_beacon();
    init_packet_unicast(identity_ft.ShortRadioDeviceID,
                        section4::mac_architecture::identity_t::ShortRadioDeviceID_reserved,
                        identity_ft.LongRadioDeviceID,
                        section4::mac_architecture::identity_t::LongRadioDeviceID_reserved);

    // ##################################################
    // DLC and Convergence Layer
    // -

    // ##################################################
    // Application Layer

    init_appiface();

    // ##################################################
    // debugging
    // -
}

std::vector<std::string> tfw_p2p_ft_t::start_threads() {
    // first start sink
    app_client->start_sc();

    // then start source
    app_server->start_sc();

    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

std::vector<std::string> tfw_p2p_ft_t::stop_threads() {
    // gracefully shut down any DECT NR+ connections, block this function until done
    // ToDo

    // close job queue so work functions will no longer be called
    job_queue.set_impermeable();

    // first stop accepting new data from upper
    app_server->stop_sc();

    // finally stop the data sink
    app_client->stop_sc();

    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

void tfw_p2p_ft_t::init_radio() {
    hw.set_command_time();
    hw.set_freq_tc(3830.0e6);

    // check what output power at 0dBFS the radio device can deliver
    TransmitPower_dBm_fixed = hw.set_tx_power_ant_0dBFS_tc(20.0f);

    // make AGC remember current power at 0dBFS, taking effect immediately
    agc_tx.set_power_ant_0dBFS_pending(TransmitPower_dBm_fixed);

    // take into consideration the OFDM crest factor
    TransmitPower_dBm_fixed += common::adt::mag2db(agc_tx.get_ofdm_amplitude_factor());

    const auto& rx_power_ant_0dBFS = hw.set_rx_power_ant_0dBFS_uniform_tc(-30.0f);

    // make AGC remember current power at 0dBFS, taking effect immediately
    agc_rx.set_power_ant_0dBFS_pending(rx_power_ant_0dBFS);
}

void tfw_p2p_ft_t::init_simulation_if_detected() {
    dectnrp_assert(hw_simulator != nullptr, "not a simulation");

    // place fixed close to origin
    const auto offset = simulation::topology::position_t::from_cartesian(0.1f, 0.1f, 0.1f);

    // add no movement
    hw_simulator->set_trajectory(simulation::topology::trajectory_t(offset));
}

void tfw_p2p_ft_t::init_packet_beacon() {
    // meta packet size
    section3::packet_sizes_def_t& psdef = ppmp_beacon.psdef;
    psdef.u = worker_pool_config.radio_device_class.u_min;
    psdef.b = worker_pool_config.radio_device_class.b_min;
    psdef.PacketLengthType = 1;
    psdef.PacketLength = 2;
#ifdef TFW_P2P_MIMO
    psdef.tm_mode_index = section3::tmmode::get_tx_div_mode(buffer_rx.nof_antennas);
#else
    psdef.tm_mode_index = 0;
#endif
    psdef.mcs_index = 2;
    psdef.Z = worker_pool_config.radio_device_class.Z_min;

    // define PLCFs
    section4::plcf_10_t& plcf_10 = ppmp_beacon.plcf_10;
    plcf_10.HeaderFormat = 0;
    plcf_10.PacketLengthType = psdef.PacketLengthType;
    plcf_10.set_PacketLength_m1(psdef.PacketLength);
    plcf_10.ShortNetworkID = identity_ft.ShortNetworkID;
    plcf_10.TransmitterIdentity = identity_ft.ShortRadioDeviceID;
    plcf_10.set_TransmitPower(TransmitPower_dBm_fixed);
    plcf_10.Reserved = 0;
    plcf_10.DFMCS = psdef.mcs_index;

    // pick one PLCF
    ppmp_beacon.plcf_base_effective = &ppmp_beacon.plcf_10;

    // define MAC header type
    ppmp_beacon.mac_header_type.Version = section4::mac_header_type_t::version_ec::v00;
    ppmp_beacon.mac_header_type.MAC_security =
        section4::mac_header_type_t::mac_security_ec::macsec_not_used;
    ppmp_beacon.mac_header_type.MAC_header_type =
        section4::mac_header_type_t::mac_header_type_ec::Beacon;

    // define MAC common header
    ppmp_beacon.beacon_header.set_Network_ID_3_lsb(identity_ft.NetworkID);
    ppmp_beacon.beacon_header.Transmitter_Address = identity_ft.LongRadioDeviceID;

    // pick one MAC common header
    ppmp_beacon.mch_base_effective = &ppmp_beacon.beacon_header;

    // set values in cluster beacon IE
    auto& cbm = mmie_pool_tx.get<section4::cluster_beacon_message_t>();
    cbm.system_frame_number = 0;
    cbm.clusters_max_tx_power =
        section4::network_beacon_message_t::clusters_max_tx_power_t::_19_dBm;
    cbm.has_power_constraints = true;
    cbm.frame_offset.reset();
    cbm.next_cluster_channel.reset();
    cbm.time_to_next.reset();
    cbm.network_beacon_period =
        section4::network_beacon_message_t::network_beacon_period_t::_100_ms;
    cbm.cluster_beacon_period = section4::network_beacon_message_t::cluster_beacon_period_t::_10_ms;
    cbm.count_to_trigger = section4::cluster_beacon_message_t::count_to_trigger_t::_1_times;
    cbm.rel_quality = section4::cluster_beacon_message_t::quality_threshold_t::_9_dB;
    cbm.min_quality = section4::cluster_beacon_message_t::quality_threshold_t::_9_dB;
}

void tfw_p2p_ft_t::init_appiface() {
#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
    // we need to define the TUN interface
    application::vnic::vnic_server_t::vnic_config_t vnic_config;

    // if multiple radio devices started on the same computer, the name has to be unique
    vnic_config.tun_name = "tundectnrp_ft_" + std::to_string(tpoint_config.firmware_id);
    vnic_config.MTU = 1500;

    // if not a simulation, we start on different computers and use unique IPs in the same network
    if (hw_simulator == nullptr) {
        vnic_config.ip_address = "172.99.180." + std::to_string(50 + tpoint_config.firmware_id);
    }
    // if a simulation, we start on the same computers and use unique networks
    else {
        vnic_config.ip_address =
            "172.99." + std::to_string(50 + tpoint_config.firmware_id) + ".180";
    }

    vnic_config.netmask = "255.255.255.0";

    app_server = std::make_unique<application::vnic::vnic_server_t>(
        id, tpoint_config.app_server_thread_config, job_queue, vnic_config, 20UL, 1600UL);

    // we need a pointer to the deriving class
    const application::vnic::vnic_server_t* vnics =
        static_cast<application::vnic::vnic_server_t*>(app_server.get());

    app_client =
        std::make_unique<application::vnic::vnic_client_t>(id,
                                                           tpoint_config.app_client_thread_config,
                                                           job_queue,
                                                           vnics->get_tuntap_fd(),
                                                           10UL,
                                                           1600UL);
#else
    // init ports for every single PT
    std::vector<uint32_t> ports_in(N_pt);
    std::vector<uint32_t> ports_out(N_pt);
    for (uint32_t i = 0; i < N_pt; ++i) {
        ports_in.at(i) = 8000 + i;
        ports_out.at(i) = 8050 + i;
    }

    app_server = std::make_unique<application::sockets::socket_server_t>(
        id, tpoint_config.app_server_thread_config, job_queue, ports_in, 4UL, 1500UL);

    app_client = std::make_unique<application::sockets::socket_client_t>(
        id, tpoint_config.app_client_thread_config, job_queue, ports_out, 4UL, 1500UL);
#endif

    // app_server->set_job_queue_access_protection_ns();
}

}  // namespace dectnrp::upper::tfw::p2p
