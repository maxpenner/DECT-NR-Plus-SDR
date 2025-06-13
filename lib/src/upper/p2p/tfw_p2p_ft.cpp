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

namespace dectnrp::upper::tfw::p2p {

const std::string tfw_p2p_ft_t::firmware_name("p2p_ft");

tfw_p2p_ft_t::tfw_p2p_ft_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    init_radio();

    if (dynamic_cast<radio::hw_simulator_t*>(&hw) != nullptr) {
        init_simulation_if_detected();
    }

    init_appiface();

    // same callback for every state
    state_t::leave_callback_t leave_callback = std::bind(&tfw_p2p_ft_t::state_transitions, this);

    // common arguments of all states
    args_t args{.tpoint_config = tpoint_config_,
                .mac_lower = mac_lower_,
                .leave_callback = leave_callback,
                .rd = rd};

    resource = std::make_unique<tfw::p2p::resource_t>(args, ft);

    steady_ft = std::make_unique<tfw::p2p::steady_ft_t>(args, ft);

    nop = std::make_unique<tfw::p2p::nop_t>(args);

    // set first state
    state = steady_ft.get();
}

phy::irregular_report_t tfw_p2p_ft_t::work_start(const int64_t start_time_64) {
    return state->work_start(start_time_64);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_regular(const phy::regular_report_t& regular_report) {
    return state->work_regular(regular_report);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_irregular(const phy::irregular_report_t& irregular_report) {
    return state->work_irregular(irregular_report);
}

phy::maclow_phy_t tfw_p2p_ft_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    return state->work_pcc(phy_maclow);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    return state->work_pdc_async(phy_machigh);
}

phy::machigh_phy_t tfw_p2p_ft_t::work_application(
    const application::application_report_t& application_report) {
    return state->work_application(application_report);
}

phy::machigh_phy_tx_t tfw_p2p_ft_t::work_chscan_async(const phy::chscan_t& chscan) {
    return state->work_chscan_async(chscan);
}

void tfw_p2p_ft_t::work_stop() {
    // gracefully shut down any DECT NR+ connections, block this function until done
    state->work_stop();

    // close job queue so work functions will no longer be called
    job_queue.set_impermeable();

    // first stop accepting new data from upper
    rd.application_server->stop_sc();

    // finally stop the data sink
    rd.application_client->stop_sc();
}

void tfw_p2p_ft_t::init_radio() {
    hw.set_command_time();
    hw.set_freq_tc(3830.0e6);

    // check what output power at 0dBFS the radio device can deliver
    ft.TransmitPower_dBm_fixed = hw.set_tx_power_ant_0dBFS_tc(20.0f);

    // make AGC remember current power at 0dBFS, taking effect immediately
    agc_tx.set_power_ant_0dBFS_pending(ft.TransmitPower_dBm_fixed);

    // take into consideration the OFDM crest factor
    ft.TransmitPower_dBm_fixed += common::adt::mag2db(agc_tx.get_ofdm_amplitude_factor());

    const auto& rx_power_ant_0dBFS = hw.set_rx_power_ant_0dBFS_uniform_tc(-40.0f);

    // make AGC remember current power at 0dBFS, taking effect immediately
    agc_rx.set_power_ant_0dBFS_pending(rx_power_ant_0dBFS);
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

void tfw_p2p_ft_t::state_transitions() {}

}  // namespace dectnrp::upper::tfw::p2p
