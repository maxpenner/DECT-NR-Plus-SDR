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

#include "dectnrp/upper/p2p/tfw_p2p_ft.hpp"

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
#include "dectnrp/application/vnic/vnic_client.hpp"
#include "dectnrp/application/vnic/vnic_server.hpp"
#else
#include "dectnrp/application/socket/socket_client.hpp"
#include "dectnrp/application/socket/socket_server.hpp"
#endif

#include "dectnrp/common/adt/decibels.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/radio/hw_simulator.hpp"

namespace dectnrp::upper::tfw::p2p {

const std::string tfw_p2p_ft_t::firmware_name("p2p_ft");

tfw_p2p_ft_t::tfw_p2p_ft_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tfw_p2p_rd_t(tpoint_config_, mac_lower_) {
    init_radio();

    if (dynamic_cast<radio::hw_simulator_t*>(&hw) != nullptr) {
        init_simulation_if_detected();
    }

    init_appiface();

    // same callback for every state
    tpoint_state_t::state_transitions_cb_t state_transitions_cb(
        std::bind(&tfw_p2p_ft_t::state_transitions, this));

    // common arguments of all states
    args_t args{.tpoint_config = tpoint_config_,
                .mac_lower = mac_lower_,
                .state_transitions_cb = state_transitions_cb,
                .rd = rd};

    resource = std::make_unique<resource_t>(args, ft);
    steady_ft = std::make_unique<steady_ft_t>(args, ft);
    dissociation_ft = std::make_unique<dissociation_ft_t>(args, ft);
    nop = std::make_unique<nop_t>(args);

    // first state
    tpoint_state = resource.get();
}

phy::machigh_phy_t tfw_p2p_ft_t::work_regular(const phy::regular_report_t& regular_report) {
    return tpoint_state->work_regular(regular_report);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_irregular(const phy::irregular_report_t& irregular_report) {
    return tpoint_state->work_irregular(irregular_report);
}

phy::maclow_phy_t tfw_p2p_ft_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    return tpoint_state->work_pcc(phy_maclow);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_pdc(const phy::phy_machigh_t& phy_machigh) {
    return tpoint_state->work_pdc(phy_machigh);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_pdc_error(const phy::phy_machigh_t& phy_machigh) {
    return tpoint_state->work_pdc_error(phy_machigh);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_application(
    const application::application_report_t& application_report) {
    return tpoint_state->work_application(application_report);
}

phy::machigh_phy_tx_t tfw_p2p_ft_t::work_channel(const phy::chscan_t& chscan) {
    return tpoint_state->work_channel(chscan);
}

void tfw_p2p_ft_t::init_radio() {
    hw.set_command_time();
    hw.set_freq_tc(3830.0e6);

    const auto& tx_power_ant_0dBFS = hw.set_tx_power_ant_0dBFS_uniform_tc(20.0f);
    ft.TransmitPower_dBm = tx_power_ant_0dBFS.at(0);
    ft.TransmitPower_dBm += common::adt::mag2db(agc_tx.get_ofdm_amplitude_factor());

    const auto& rx_power_ant_0dBFS = hw.set_rx_power_ant_0dBFS_uniform_tc(-40.0f);
    ft.ReceiverPower_dBm = rx_power_ant_0dBFS.at(0);
    ft.ReceiverPower_dBm += common::adt::mag2db(agc_rx.get_rms_target());
}

void tfw_p2p_ft_t::init_simulation_if_detected() {
    radio::hw_simulator_t* hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

    dectnrp_assert(hw_simulator != nullptr, "not a simulation");

    // place fixed close to origin
    const auto offset = simulation::topology::position_t::from_cartesian(0.1f, 0.1f, 0.1f);

    // add no movement
    hw_simulator->set_trajectory(simulation::topology::trajectory_t(offset));
}

void tfw_p2p_ft_t::init_appiface() {
#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
    // we need to define the TUN interface
    application::vnic::vnic_server_t::vnic_config_t vnic_config;

    // if multiple radio devices started on the same computer, the name has to be unique
    vnic_config.tun_name = "tun_ft_" + std::to_string(tpoint_config.firmware_id);
    vnic_config.MTU = 1500;

    // if not a simulation, we start on different computers and use unique IPs in the same network
    if (dynamic_cast<radio::hw_simulator_t*>(&hw) == nullptr) {
        vnic_config.ip_address = "172.99.180." + std::to_string(50 + tpoint_config.firmware_id);
    }
    // if a simulation, we start on the same computers and use unique networks
    else {
        vnic_config.ip_address =
            "172.99." + std::to_string(50 + tpoint_config.firmware_id) + ".180";
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

    // we need a pointer to the deriving class
    const application::vnic::vnic_server_t* vnics =
        static_cast<application::vnic::vnic_server_t*>(rd.application_server.get());

    const application::queue_size_t queue_size_client = {
        .N_datagram = 10, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    rd.application_client = std::make_unique<application::vnic::vnic_client_t>(
        id,
        tpoint_config.application_client_thread_config,
        job_queue,
        vnics->get_tuntap_fd(),
        queue_size_client);
#else
    // init ports for every single PT
    std::vector<uint32_t> ports_in(rd.N_pt);
    std::vector<uint32_t> ports_out(rd.N_pt);
    for (uint32_t i = 0; i < rd.N_pt; ++i) {
        ports_in.at(i) = 8000 + i;
        ports_out.at(i) = 8050 + i;
    }

    const application::queue_size_t queue_size = {
        .N_datagram = 4, .N_datagram_max_byte = limits::application_max_queue_datagram_byte};

    rd.application_server = std::make_unique<application::sockets::socket_server_t>(
        id, tpoint_config.application_server_thread_config, job_queue, ports_in, queue_size);

    rd.application_client = std::make_unique<application::sockets::socket_client_t>(
        id, tpoint_config.application_client_thread_config, job_queue, ports_out, queue_size);
#endif

    // application_server->set_job_queue_access_protection_ns();

    // first start sink
    rd.application_client->start_sc();

    // then start source
    rd.application_server->start_sc();
}

phy::irregular_report_t tfw_p2p_ft_t::state_transitions() {
    phy::irregular_report_t ret;

    if (tpoint_state == resource.get()) {
        ret = steady_ft->entry();
        tpoint_state = steady_ft.get();

        if (rd.is_shutting_down()) {
            ret = dissociation_ft->entry();
            tpoint_state = dissociation_ft.get();
        }

    } else if (tpoint_state == steady_ft.get()) {
        ret = dissociation_ft->entry();
        tpoint_state = dissociation_ft.get();
    } else if (tpoint_state == dissociation_ft.get()) {
        tpoint_state = nop.get();
        stop_request_unblock();
    } else if (tpoint_state == nop.get()) {
        // nop state must not be exited
    } else {
        dectnrp_assert_failure("undefined state");
    }

    return ret;
}

}  // namespace dectnrp::upper::tfw::p2p
