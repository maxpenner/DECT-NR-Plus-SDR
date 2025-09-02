/*
 * Copyright 2023-present Maxim Penner
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

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
#include "dectnrp/application/vnic/vnic_client.hpp"
#include "dectnrp/application/vnic/vnic_server.hpp"
#else
#include "dectnrp/application/socket/socket_client.hpp"
#include "dectnrp/application/socket/socket_server.hpp"
#endif

#include "dectnrp/radio/hw_simulator.hpp"

namespace dectnrp::upper::tfw::p2p {

const std::string tfw_p2p_pt_t::firmware_name("p2p_pt");

tfw_p2p_pt_t::tfw_p2p_pt_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tfw_p2p_rd_t(tpoint_config_, mac_lower_) {
    init_radio();

    if (dynamic_cast<radio::hw_simulator_t*>(&hw) != nullptr) {
        init_simulation_if_detected();
    }

    init_appiface();

    // same callback for every state
    tpoint_state_t::state_transitions_cb_t state_transitions_cb(
        std::bind(&tfw_p2p_pt_t::state_transitions, this));

    // common arguments of all states
    args_t args{.tpoint_config = tpoint_config_,
                .mac_lower = mac_lower_,
                .state_transitions_cb = state_transitions_cb,
                .rd = rd};

    association = std::make_unique<association_t>(args, pt);
    steady_pt = std::make_unique<steady_pt_t>(args, pt);
    dissociation_pt = std::make_unique<dissociation_pt_t>(args, pt);
    nop = std::make_unique<nop_t>(args);

    // first state
    tpoint_state = association.get();
}

phy::machigh_phy_t tfw_p2p_pt_t::work_regular(const phy::regular_report_t& regular_report) {
    return tpoint_state->work_regular(regular_report);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_irregular(const phy::irregular_report_t& irregular_report) {
    return tpoint_state->work_irregular(irregular_report);
}

phy::maclow_phy_t tfw_p2p_pt_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    return tpoint_state->work_pcc(phy_maclow);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_pdc(const phy::phy_machigh_t& phy_machigh) {
    return tpoint_state->work_pdc(phy_machigh);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_pdc_error(const phy::phy_machigh_t& phy_machigh) {
    return tpoint_state->work_pdc_error(phy_machigh);
}

phy::machigh_phy_t tfw_p2p_pt_t::work_application(
    const application::application_report_t& application_report) {
    return tpoint_state->work_application(application_report);
}

phy::machigh_phy_tx_t tfw_p2p_pt_t::work_channel(const phy::chscan_t& chscan) {
    return tpoint_state->work_channel(chscan);
}

void tfw_p2p_pt_t::init_radio() {
    hw.set_command_time();
    hw.set_freq_tc(3830.0e6);
    hw.set_tx_power_ant_0dBFS_uniform_tc(10.0f);
    hw.set_rx_power_ant_0dBFS_uniform_tc(-40.0f);
}

void tfw_p2p_pt_t::init_simulation_if_detected() {
    radio::hw_simulator_t* hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

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
    if (dynamic_cast<radio::hw_simulator_t*>(&hw) == nullptr) {
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

    rd.application_server = std::make_unique<application::vnic::vnic_server_t>(
        id,
        tpoint_config.application_server_thread_config,
        job_queue,
        vnic_config,
        queue_size_server);

    application::vnic::vnic_server_t* vnics =
        dynamic_cast<application::vnic::vnic_server_t*>(rd.application_server.get());

    dectnrp_assert(vnics != nullptr, "vnic server cast failed");

    const application::queue_size_t queue_size_client = {
        .N_datagram = 10, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    rd.application_client = std::make_unique<application::vnic::vnic_client_t>(
        id,
        tpoint_config.application_client_thread_config,
        job_queue,
        vnics->get_tuntap_fd(),
        queue_size_client);
#else
    const application::queue_size_t queue_size = {
        .N_datagram = 4, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    rd.application_server = std::make_unique<application::sockets::socket_server_t>(
        id,
        tpoint_config.application_server_thread_config,
        job_queue,
        std::vector<uint32_t>{8100 + tpoint_config.firmware_id},
        queue_size);

    rd.application_client = std::make_unique<application::sockets::socket_client_t>(
        id,
        tpoint_config.application_client_thread_config,
        job_queue,
        std::vector<uint32_t>{8150 + tpoint_config.firmware_id},
        queue_size);
#endif

    // application_server->set_job_queue_access_protection_ns();

    // first start sink
    rd.application_client->start_sc();

    // then start source
    rd.application_server->start_sc();
}

phy::irregular_report_t tfw_p2p_pt_t::state_transitions() {
    phy::irregular_report_t ret;

    if (tpoint_state == association.get()) {
        ret = steady_pt->entry();
        tpoint_state = steady_pt.get();

        if (rd.is_shutting_down()) {
            ret = dissociation_pt->entry();
            tpoint_state = dissociation_pt.get();
        }

    } else if (tpoint_state == steady_pt.get()) {
        ret = dissociation_pt->entry();
        tpoint_state = dissociation_pt.get();
    } else if (tpoint_state == dissociation_pt.get()) {
        tpoint_state = nop.get();
        stop_request_unblock();
    } else if (tpoint_state == nop.get()) {
        // state must not be exited
    } else {
        dectnrp_assert_failure("undefined state");
    }

    return ret;
}

}  // namespace dectnrp::upper::tfw::p2p
