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

#include "dectnrp/simulation/vspace.hpp"

#include <algorithm>
#include <cmath>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/simulation/topology/graph.hpp"
#include "dectnrp/simulation/wireless/channel_awgn.hpp"
#include "dectnrp/simulation/wireless/channel_doubly.hpp"
#include "dectnrp/simulation/wireless/channel_flat.hpp"

namespace dectnrp::simulation {

vspace_t::vspace_t(const uint32_t nof_hw_simulator_,
                   const uint32_t samp_rate_speedup_,
                   const std::string sim_channel_name_inter_,
                   const std::string sim_channel_name_intra_,
                   const std::string sim_noise_type_)
    : nof_hw_simulator(nof_hw_simulator_),
      samp_rate_speedup(samp_rate_speedup_),
      sim_channel_name_inter(sim_channel_name_inter_),
      sim_channel_name_intra(sim_channel_name_intra_),
      sim_noise_type(sim_noise_type_) {
    // block threads at wait_registration_nto()
    all_tx_registered_and_inits_done = false;
    all_rx_registered_and_inits_done = false;

    status_tx_written.resize(nof_hw_simulator);
    status_rx_read.resize(nof_hw_simulator);

    vspptx_vec.resize(nof_hw_simulator);

    // convert string to enum class
    if (sim_noise_type_ == "relative") {
        noise_type = NOISE_TYPE_t::relative;
    } else if (sim_noise_type_ == "thermal") {
        noise_type = NOISE_TYPE_t::thermal;
    } else {
        dectnrp_assert_failure("Unknown noise type.");
    }

    wchannel_randomgen.shuffle();

    wchannel_noise_source = std::make_unique<noise_t>(wchannel_randomgen.randi(0, UINT32_MAX - 1));

    // as for the topology, we assume a complete graph
    wchannel_inter_vec.resize(topology::complete_graph_nof_edges(nof_hw_simulator));
    wchannel_intra_vec.resize(nof_hw_simulator);
}

void vspace_t::hw_register_tx(const vspptx_t& vspptx) {
    std::unique_lock<std::mutex> lk(mtx);

    dectnrp_assert(vspptx.id < nof_hw_simulator, "HW out of range.");
    dectnrp_assert(vspptx_vec[vspptx.id].get() == nullptr, "HW already registered.");

    // init tx vspp counterpart, to which TX threads will deepcopy
    vspptx_vec[vspptx.id] = std::make_unique<vspptx_t>(
        vspptx.id, vspptx.nof_antennas, vspptx.samp_rate, vspptx.spp_size);

    // all devices registered?
    if (check_if_last_to_register_tx()) {
        // extract common values, assume the first device has the reference values
        samp_rate_common = vspptx_vec[0]->samp_rate;
        spp_size_common = vspptx_vec[0]->spp_size;

        // make sure all simulators use the same values
        for (uint32_t i = 1; i < nof_hw_simulator; ++i) {
            dectnrp_assert(samp_rate_common == vspptx_vec[i]->samp_rate,
                           "HW hw_samp_rate incorrect.");
            dectnrp_assert(spp_size_common == vspptx_vec[i]->spp_size, "HW spp_size incorrect.");
        }

        // create edges of complete graph
        wchannel_generate_graph();

        // allow all TX and RX threads to work and wake them up
        all_tx_registered_and_inits_done = true;
        cv_all_tx_registered_and_inits_done.notify_all();
    }
}

void vspace_t::hw_register_rx(const vspprx_t& vspprx) {
    std::unique_lock<std::mutex> lk(mtx);

    dectnrp_assert(!vspptx_vec[vspprx.id]->meta.vspprx_counterpart_registered,
                   "HW counterpart already registered.");

    // set as registered
    vspptx_vec[vspprx.id]->meta.vspprx_counterpart_registered = true;

    dectnrp_assert(vspprx.samp_rate == vspptx_vec[vspprx.id]->samp_rate,
                   "HW hw_samp_rate incorrect.");
    dectnrp_assert(vspprx.spp_size == vspptx_vec[vspprx.id]->spp_size, "HW spp_size incorrect.");

    // all devices registered?
    if (check_if_last_to_register_rx()) {
        // allow TX threads to write to virtual space, but block RX threads from reading
        set_status_tx_written(false);
        set_status_rx_read(true);

        // realign real time and virtual time at zero
        watch.reset();
        now_64 = 0;

        // allow all TX and RX threads to work and wake them up
        all_rx_registered_and_inits_done = true;
        cv_all_rx_registered_and_inits_done.notify_all();
    }
}

void vspace_t::wait_for_all_tx_registered_and_inits_done_nto() {
    std::unique_lock<std::mutex> lk(mtx);

    while (!all_tx_registered_and_inits_done) {
        cv_all_tx_registered_and_inits_done.wait_for(
            lk, std::chrono::milliseconds(TXRX_WAIT_TIMEOUT_MS));
    }
}

void vspace_t::wait_for_all_rx_registered_and_inits_done_nto() {
    std::unique_lock<std::mutex> lk(mtx);

    while (!all_rx_registered_and_inits_done) {
        cv_all_rx_registered_and_inits_done.wait_for(
            lk, std::chrono::milliseconds(TXRX_WAIT_TIMEOUT_MS));
    }
}

bool vspace_t::wait_writable_to(const uint32_t id) {
    std::unique_lock<std::mutex> lk(mtx);

    while (status_tx_written[id]) {
        auto res = cv_tx.wait_for(lk, std::chrono::milliseconds(TXRX_WAIT_TIMEOUT_MS));

        // timeout, leave to check global exit condition
        if (res == std::cv_status::timeout) {
            return false;
        }
    }

    return true;
}

void vspace_t::wait_writable_nto(const uint32_t id) {
    while (!wait_writable_to(id)) {
        // nothing to do here
    }
}

bool vspace_t::wait_readable_to(const uint32_t id) {
    std::unique_lock<std::mutex> lk(mtx);

    while (status_rx_read[id]) {
        auto res = cv_rx.wait_for(lk, std::chrono::milliseconds(TXRX_WAIT_TIMEOUT_MS));

        // timeout, leave to check global exit condition
        if (res == std::cv_status::timeout) {
            return false;
        }
    }

    return true;
}

void vspace_t::write(vspptx_t& vspptx) {
    std::unique_lock<std::mutex> lk(mtx);

    dectnrp_assert(!status_tx_written[vspptx.id], "HW TX interface already written");
    dectnrp_assert(vspptx.meta.now_64 == now_64, "HW time not the same as vspace time");

    /* An instance of hw_simulator has generated its TX signal as it leaves the antenna, and written
     * it to vspptx. With this signal, the TX thread of hw_simulator has now called this function
     * and attained an exlusive lock on the virtual space. Before deepcopying its signal to
     * vspptx_vec, we change our state in the virtual space.
     */

    // change position
    vspptx.meta.trajectory.update_position(vspptx.meta.position, samp_rate_common, now_64);

    // make deep copy from TX thread to virtual space
    vspptx.deepcopy(*vspptx_vec[vspptx.id].get());

    // set as written
    status_tx_written[vspptx.id] = true;

    // check whether this is the last call
    if (check_if_last_to_write()) {
        // if so, set all RX buffers as not read ...
        set_status_rx_read(false);

        // ... and trigger the RX threads
        cv_rx.notify_all();
    }
}

void vspace_t::read(vspprx_t& vspprx) {
    std::unique_lock<std::mutex> lk(mtx);

    dectnrp_assert(!status_rx_read[vspprx.id], "HW RX interface already read");
    dectnrp_assert(vspprx.meta.now_64 == now_64, "HW time not the same as vspace time");

    // we fill vspprx
    wchannel_execute(vspprx);

    // set as read
    status_rx_read[vspprx.id] = true;

    // check whether this is the last device to read its RX data
    if (check_if_last_to_read()) {
        // increase system time
        now_64 += static_cast<int64_t>(spp_size_common);

        // last RX thread tries to realign time axles
        realign_realtime_with_simulationtime(
            watch, samp_rate_speedup, static_cast<int64_t>(samp_rate_common), now_64);

        // set all TX buffers as not written ...
        set_status_tx_written(false);

        // ... and trigger the TX threads
        cv_tx.notify_all();
    }
}

void vspace_t::wchannel_randomize_small_scale() {
    std::unique_lock<std::mutex> lk(mtx);

    for (uint32_t i = 0; i < wchannel_inter_vec.size(); ++i) {
        wchannel_inter_vec[i]->randomize_small_scale();
    }

    for (uint32_t i = 0; i < wchannel_intra_vec.size(); ++i) {
        wchannel_intra_vec[i]->randomize_small_scale();
    }
}

void vspace_t::realign_realtime_with_simulationtime(const common::watch_t& watch,
                                                    const int32_t samp_rate_speedup,
                                                    const int64_t samp_rate_64,
                                                    const int64_t now_simulation_64) {
    // determine how many samples we should have created by now in real time
    const int64_t now_realtime_target_64 =
        (samp_rate_speedup >= 0) ? watch.get_elapsed<int64_t, common::milli>() * samp_rate_64 *
                                       int64_t{10 + samp_rate_speedup} / int64_t{10} / int64_t{1000}
                                 : watch.get_elapsed<int64_t, common::milli>() * samp_rate_64 /
                                       int64_t{-samp_rate_speedup} / int64_t{1000};

    /* Sleep until realtime has caught up with simulation time. This only makes sense if the CPU
     * processes the simulation faster than realtime. Otherwise this section is skipped and the
     * ratio between the time axles is NOT samp_rate_speedup.
     */
    if (now_realtime_target_64 < now_simulation_64) {
        const int64_t delta_us_64 =
            (now_simulation_64 - now_realtime_target_64) * int64_t{1000000} / samp_rate_64;

        common::watch_t::sleep<common::micro>(delta_us_64);
    }
}

bool vspace_t::check_if_last_to_register_tx() const {
    for (auto& elem : vspptx_vec) {
        if (elem.get() == nullptr) {
            return false;
        }
    }

    return true;
}

bool vspace_t::check_if_last_to_register_rx() const {
    if (!all_tx_registered_and_inits_done) {
        return false;
    }

    for (auto& elem : vspptx_vec) {
        if (!elem->meta.vspprx_counterpart_registered) {
            return false;
        }
    }

    return true;
}

bool vspace_t::check_if_last_to_write() const {
    if (std::find(status_tx_written.begin(), status_tx_written.end(), false) !=
        status_tx_written.end()) {
        return false;
    }

    return true;
}

bool vspace_t::check_if_last_to_read() const {
    if (std::find(status_rx_read.begin(), status_rx_read.end(), false) != status_rx_read.end()) {
        return false;
    }

    return true;
}

void vspace_t::set_status_tx_written(const bool val) {
    std::fill(status_tx_written.begin(), status_tx_written.end(), val);
}

void vspace_t::set_status_rx_read(const bool val) {
    std::fill(status_rx_read.begin(), status_rx_read.end(), val);
}

void vspace_t::wchannel_generate_graph() {
    /* wchannel_inter_vec
     *
     * Refer to description of complete_graph_sorted_edge_index() for explanation of indices.
     */
    uint32_t edge_cnt = 0;
    for (uint32_t i = 0; i < nof_hw_simulator - 1; ++i) {
        for (uint32_t j = i + 1; j < nof_hw_simulator; ++j) {
            dectnrp_assert(
                topology::complete_graph_sorted_edge_index(nof_hw_simulator, i, j) == edge_cnt &&
                    topology::complete_graph_sorted_edge_index(nof_hw_simulator, j, i) == edge_cnt,
                "Incorrect edge index.");

            // instantiate same channel for each edge
            if (sim_channel_name_inter == channel_awgn_t::name) {
                wchannel_inter_vec[edge_cnt] =
                    std::make_unique<channel_awgn_t>(i, j, samp_rate_common, spp_size_common);

            } else if (sim_channel_name_inter.substr(0, channel_doubly_t::name.size()) ==
                       channel_doubly_t::name) {
                wchannel_inter_vec[edge_cnt] =
                    std::make_unique<channel_doubly_t>(i,
                                                       j,
                                                       samp_rate_common,
                                                       spp_size_common,
                                                       vspptx_vec[i]->nof_antennas,
                                                       vspptx_vec[j]->nof_antennas,
                                                       sim_channel_name_inter);
            } else if (sim_channel_name_inter.substr(0, channel_flat_t::name.size()) ==
                       channel_flat_t::name) {
                wchannel_inter_vec[edge_cnt] =
                    std::make_unique<channel_flat_t>(i,
                                                     j,
                                                     samp_rate_common,
                                                     spp_size_common,
                                                     vspptx_vec[i]->nof_antennas,
                                                     vspptx_vec[j]->nof_antennas);
            } else {
                dectnrp_assert_failure("Unknown channel name.");
            }

            ++edge_cnt;
        }
    }

    dectnrp_assert(edge_cnt == wchannel_inter_vec.size(), "Incorrect number of edges.");

    // wchannel_intra_vec
    for (uint32_t i = 0; i < nof_hw_simulator; ++i) {
        // instantiate same channel for each edge
        if (sim_channel_name_intra == channel_awgn_t::name) {
            wchannel_intra_vec[i] =
                std::make_unique<channel_awgn_t>(i, i, samp_rate_common, spp_size_common);

        } else if (sim_channel_name_intra.substr(0, channel_doubly_t::name.size()) ==
                   channel_doubly_t::name) {
            wchannel_intra_vec[i] = std::make_unique<channel_doubly_t>(i,
                                                                       i,
                                                                       samp_rate_common,
                                                                       spp_size_common,
                                                                       vspptx_vec[i]->nof_antennas,
                                                                       vspptx_vec[i]->nof_antennas,
                                                                       sim_channel_name_intra);
        } else if (sim_channel_name_intra.substr(0, channel_flat_t::name.size()) ==
                   channel_flat_t::name) {
            wchannel_intra_vec[i] = std::make_unique<channel_flat_t>(i,
                                                                     i,
                                                                     samp_rate_common,
                                                                     spp_size_common,
                                                                     vspptx_vec[i]->nof_antennas,
                                                                     vspptx_vec[i]->nof_antennas);
        } else {
            dectnrp_assert_failure("Unknown channel name.");
        }
    }
}

void vspace_t::wchannel_execute(vspprx_t& vspprx) const {
    // step 0: create reference to own transmission
    const vspptx_t& vspptx = *vspptx_vec[vspprx.id].get();

    // step 1: zero before superposition
    vspprx.spp_zero();

    // step 2: inter-simulator wireless transmission
    for (uint32_t i = 0; i < nof_hw_simulator; ++i) {
        // ignore yourself
        if (i == vspprx.id) {
            continue;
        }

        // lookup correct channel index a.k.a. edge index
        const uint32_t idx =
            topology::complete_graph_sorted_edge_index(nof_hw_simulator, vspprx.id, i);

        // pass transmitted spp through channel and add output to received spp
        wchannel_inter_vec[idx]->superimpose(vspptx, vspprx, *vspptx_vec[i].get());
    }

    /* step 3: intra-simulator TX/RX switching
     *
     * We use the reference to our own transmission to check whether it contains non-zero TX
     * samples. If so, then the RX samples during this TX period are set to zero. Like in real
     * hardware, the RX path is detached from the TX/RX antenna while a transmission is ongoing.
     */
    if (vspptx.tx_idx >= 0) {
        vspprx.spp_zero(vspptx.tx_idx, vspptx.tx_length);
    }

    /* step 4: intra-simulator TX into RX leakage
     *
     * We assume that a specific TX antenna only leaks into the corresponding RX path. Whether this
     * holds true for a device depends on the hardware structure, e.g. whether each TX/RX antenna is
     * placed on its own daughter board.
     */
    wchannel_intra_vec[vspprx.id]->superimpose(vspptx, vspprx, vspptx);

    // step 5: add thermal noise after superposition
    if (noise_type == NOISE_TYPE_t::relative) {
        // add noise across full bandwidth
        for (size_t i = 0; i < vspprx.nof_antennas; ++i) {
            wchannel_noise_source->awgn(vspprx.spp.at(i),
                                        vspprx.spp.at(i),
                                        vspprx.spp_size,
                                        vspptx.meta.net_bandwidth_norm,
                                        vspprx.meta.rx_snr_in_net_bandwidth_norm_dB);
        }
    } else if (noise_type == NOISE_TYPE_t::thermal) {
        // calculate absolute noise power
        const float noise_power_dBm = -174.0f + 10.0f * log10(static_cast<float>(samp_rate_common));

        // add noise across full bandwidth
        for (size_t i = 0; i < vspprx.nof_antennas; ++i) {
            wchannel_noise_source->awgn(vspprx.spp.at(i),
                                        vspprx.spp.at(i),
                                        vspprx.spp_size,
                                        1.0f,
                                        vspprx.meta.rx_power_ant_0dBFS.at(i) - noise_power_dBm -
                                            vspprx.meta.rx_noise_figure_dB);
        }
    }
}

}  // namespace dectnrp::simulation
