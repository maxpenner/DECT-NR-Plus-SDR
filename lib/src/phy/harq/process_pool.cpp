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

#include "dectnrp/phy/harq/process_pool.hpp"

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy::harq {

process_pool_t::process_pool_t(const sp3::packet_sizes_t maximum_packet_sizes,
                               const uint32_t nof_process_tx,
                               const uint32_t nof_process_rx) {
    for (uint32_t i = 0; i < nof_process_tx; ++i) {
        hp_tx_vec.push_back(std::make_unique<process_tx_t>(i, maximum_packet_sizes));
    }

    for (uint32_t i = 0; i < nof_process_rx; ++i) {
        hp_rx_vec.push_back(std::make_unique<process_rx_t>(i, maximum_packet_sizes));
    }
}

process_tx_t* process_pool_t::get_process_tx(const uint32_t PLCF_type,
                                             const uint32_t network_id,
                                             const sp3::packet_sizes_def_t psdef,
                                             const finalize_tx_t ftx) const {
    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "unknown PLCF type");

    const auto packet_sizes_opt = sp3::get_packet_sizes(psdef);

    dectnrp_assert(packet_sizes_opt.has_value(), "packets must be well-defined");

    // go over each HARQ process ...
    for (uint32_t i = 0; i < hp_tx_vec.size(); ++i) {
        // ... and try to lock it
        if (hp_tx_vec.at(i)->try_lock_outer()) {
            dectnrp_assert(packet_sizes_opt->N_TB_byte <= hp_tx_vec[i]->get_hb_tb()->a_len,
                           "packet TB larger than buffer");

            hp_tx_vec[i]->PLCF_type = PLCF_type;
            hp_tx_vec[i]->network_id = network_id;
            hp_tx_vec[i]->packet_sizes = packet_sizes_opt.value();
            hp_tx_vec[i]->finalize_tx = ftx;
            hp_tx_vec[i]->lock_inner();

            return hp_tx_vec[i].get();
        }
    }

    return nullptr;
};

process_rx_t* process_pool_t::get_process_rx(const uint32_t PLCF_type,
                                             const uint32_t network_id,
                                             const sp3::packet_sizes_def_t psdef,
                                             uint32_t rv,
                                             const finalize_rx_t frx) const {
    dectnrp_assert(PLCF_type == 1 || PLCF_type == 2, "unknown PLCF type");

    const auto packet_sizes_opt = sp3::get_packet_sizes(psdef);

    dectnrp_assert(packet_sizes_opt.has_value(), "packets must be well-defined");

    // go over each HARQ process ...
    for (uint32_t i = 0; i < hp_rx_vec.size(); ++i) {
        // ... and try to lock it
        if (hp_rx_vec.at(i)->try_lock_outer()) {
            dectnrp_assert(packet_sizes_opt->N_TB_byte <= hp_rx_vec[i]->get_hb_tb()->a_len,
                           "packet TB larger than buffer");

            hp_rx_vec[i]->PLCF_type = PLCF_type;
            hp_rx_vec[i]->network_id = network_id;
            hp_rx_vec[i]->packet_sizes = packet_sizes_opt.value();
            hp_rx_vec[i]->rv = rv;
            hp_rx_vec[i]->finalize_rx = frx;
            hp_rx_vec[i]->lock_inner();

            return hp_rx_vec[i].get();
        }
    }

    return nullptr;
};

process_tx_t* process_pool_t::get_process_tx_running(const uint32_t id,
                                                     const finalize_tx_t ftx) const {
    dectnrp_assert(hp_tx_vec.at(id)->is_outer_locked(), "incorrect lock state");

    if (hp_tx_vec.at(id)->is_inner_locked()) {
        return nullptr;
    }

    hp_tx_vec[id]->finalize_tx = ftx;
    hp_tx_vec[id]->lock_inner();

    return hp_tx_vec[id].get();
}

process_rx_t* process_pool_t::get_process_rx_running(const uint32_t id,
                                                     uint32_t rv,
                                                     const finalize_rx_t frx) const {
    dectnrp_assert(hp_rx_vec.at(id)->is_outer_locked(), "incorrect lock state");

    if (hp_rx_vec.at(id)->is_inner_locked()) {
        return nullptr;
    }

    hp_rx_vec[id]->rv = rv;
    hp_rx_vec[id]->finalize_rx = frx;
    hp_rx_vec[id]->lock_inner();

    return hp_rx_vec[id].get();
}

}  // namespace dectnrp::phy::harq
