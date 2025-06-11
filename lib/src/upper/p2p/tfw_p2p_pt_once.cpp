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

#include "dectnrp/upper/p2p/tfw_p2p_pt.hpp"
//

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
#include "dectnrp/application/vnic/vnic_client.hpp"
#include "dectnrp/application/vnic/vnic_server.hpp"
#else
#include "dectnrp/application/socket/socket_client.hpp"
#include "dectnrp/application/socket/socket_server.hpp"
#endif

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/radio/hw_simulator.hpp"

namespace dectnrp::upper::tfw::p2p {

const std::string tfw_p2p_pt_t::firmware_name("p2p_pt");

tfw_p2p_pt_t::tfw_p2p_pt_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tfw_p2p_base_t(tpoint_config_, mac_lower_) {
    // ##################################################
    // Radio Layer + PHY

    init_radio();

    if (hw_simulator != nullptr) {
        init_simulation_if_detected();
    }

    // ##################################################
    // MAC Layer

    contact_pt.sync_report = phy::sync_report_t(buffer_rx.nof_antennas);
    contact_pt.identity = init_identity_pt(tpoint_config.firmware_id);
    contact_pt.allocation_pt = init_allocation_pt(tpoint_config.firmware_id);
    contact_pt.mimo_csi = phy::mimo_csi_t();
    contact_pt.conn_idx_server = 0;
    contact_pt.conn_idx_client = 0;

    // feedback format 4 for MCS, 5 for codebookindex
    contact_pt.feedback_plan = mac::feedback_plan_t(std::vector<uint32_t>{4, 5});

    init_packet_unicast(contact_pt.identity.ShortRadioDeviceID,
                        identity_ft.ShortRadioDeviceID,
                        contact_pt.identity.LongRadioDeviceID,
                        identity_ft.LongRadioDeviceID);

    // ##################################################
    // DLC and Convergence Layer
    // -

    // ##################################################
    // Application Layer

    init_appiface();

    // first start sink
    application_client->start_sc();

    // then start source
    application_server->start_sc();

    // ##################################################
    // debugging

    // -
}

void tfw_p2p_pt_t::shutdown() {
    // gracefully shut down any DECT NR+ connections, block this function until done
    // ToDo

    // close job queue so work functions will no longer be called
    job_queue.set_impermeable();

    // first stop accepting new data from upper
    application_server->stop_sc();

    // finally stop the data sink
    application_client->stop_sc();
}

void tfw_p2p_pt_t::init_radio() {
    hw.set_command_time();
    hw.set_freq_tc(3830.0e6);
    const float tx_power_ant_0dBFS = hw.set_tx_power_ant_0dBFS_tc(10.0f);
    const auto& rx_power_ant_0dBFS = hw.set_rx_power_ant_0dBFS_uniform_tc(-30.0f);

    // make AGCs remember current power at 0dBFS, taking effect immediately
    agc_tx.set_power_ant_0dBFS_pending(tx_power_ant_0dBFS);
    agc_rx.set_power_ant_0dBFS_pending(rx_power_ant_0dBFS);
}

void tfw_p2p_pt_t::init_simulation_if_detected() {
    dectnrp_assert(hw_simulator != nullptr, "not a simulation");

    const float firmware_id_f = static_cast<float>(tpoint_config.firmware_id);

    // place portable around origin
    const auto offset = simulation::topology::position_t::from_polar(20.0f, firmware_id_f * 180.0f);

    // add movement
    hw_simulator->set_trajectory(
        simulation::topology::trajectory_t(offset, 0.1f + 2.0f * firmware_id_f, 15.0f));
}

void tfw_p2p_pt_t::init_appiface() {
#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
    // we need to define the TUN interface
    application::vnic::vnic_server_t::vnic_config_t vnic_config;

    // if multiple radio devices started on the same computer, the name has to be unique
    vnic_config.tun_name = "tun_pt_" + std::to_string(tpoint_config.firmware_id);
    vnic_config.MTU = 1500;

    // if not a simulation, we start on different computers and use unique IPs in the same network
    if (hw_simulator == nullptr) {
        vnic_config.ip_address = "172.99.180." + std::to_string(100 + tpoint_config.firmware_id);
    }

    // if a simulation, we start on the same computers and use unique networks
    else {
        vnic_config.ip_address =
            "172.99." + std::to_string(100 + tpoint_config.firmware_id) + ".180";
    }

    vnic_config.netmask = "255.255.255.0";

    const application::queue_size_t queue_size_server = {
        .N_datagram = 20, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    application_server = std::make_unique<application::vnic::vnic_server_t>(
        id,
        tpoint_config.application_server_thread_config,
        job_queue,
        vnic_config,
        queue_size_server);

    application::vnic::vnic_server_t* vnics =
        dynamic_cast<application::vnic::vnic_server_t*>(application_server.get());

    dectnrp_assert(vnics != nullptr, "vnic server cast failed");

    const application::queue_size_t queue_size_client = {
        .N_datagram = 10, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    application_client = std::make_unique<application::vnic::vnic_client_t>(
        id,
        tpoint_config.application_client_thread_config,
        job_queue,
        vnics->get_tuntap_fd(),
        queue_size_client);
#else
    const application::queue_size_t queue_size = {
        .N_datagram = 4, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    application_server = std::make_unique<application::sockets::socket_server_t>(
        id,
        tpoint_config.application_server_thread_config,
        job_queue,
        std::vector<uint32_t>{8100 + tpoint_config.firmware_id},
        queue_size);

    application_client = std::make_unique<application::sockets::socket_client_t>(
        id,
        tpoint_config.application_client_thread_config,
        job_queue,
        std::vector<uint32_t>{8150 + tpoint_config.firmware_id},
        queue_size);
#endif

    // application_server->set_job_queue_access_protection_ns();
}

}  // namespace dectnrp::upper::tfw::p2p
