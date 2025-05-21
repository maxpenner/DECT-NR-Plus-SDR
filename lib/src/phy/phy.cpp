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

#include "dectnrp/phy/phy.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/radio/hw.hpp"
#include "dectnrp/sections_part3/derivative/duration.hpp"

namespace dectnrp::phy {

phy_t::phy_t(const phy_config_t& phy_config_, const radio::radio_t& radio_)
    : layer_t("PHY"),
      phy_config(phy_config_) {
    dectnrp_assert(radio_.get_nof_layer_unit() == phy_config.get_nof_layer_unit_config(),
                   "Number of hardware devices and number of worker pools must be the same");

    for (uint32_t wp_id = 0; wp_id < phy_config.get_nof_layer_unit_config(); ++wp_id) {
        // get hw associated with wp_id and ...
        radio::hw_t& hw = radio_.get_layer_unit(wp_id);

        // ... add one instance of radio_ctrl to give all worker_pool_t instances same control
        phy_radio.radio_ctrl_vec.emplace_back(*hw.buffer_tx_pool.get());
    }

    // configure each thread pool and its associated hardware
    for (uint32_t wp_id = 0; wp_id < phy_config.get_nof_layer_unit_config(); ++wp_id) {
        const auto& worker_pool_config = phy_config.get_layer_unit_config(wp_id);

        dectnrp_assert(worker_pool_config.id == wp_id, "incorrect ID");

        // initialize associated hardware

        // thread pool and hardware have the same id
        radio::hw_t& hw = radio_.get_layer_unit(wp_id);

        dectnrp_assert(
            worker_pool_config.radio_device_class.N_TX_min <= limits::dectnrp_max_nof_antennas,
            "number of antennas exceeds limit");

        hw.set_nof_antennas(worker_pool_config.radio_device_class.N_TX_min);

        // let hardware pick one of its sample rates
        hw.set_samp_rate(worker_pool_config.get_dect_samp_rate_max_oversampled());

        // hardware chose a sample rate, check if resampling is doable and with what L/M
        const resampler_param_t resampler_param = phy_config_t::get_resampler_param_verified(
            hw.get_samp_rate(),
            worker_pool_config.get_dect_samp_rate_max_oversampled(),
            worker_pool_config.enforce_dectnrp_samp_rate_by_resampling);

        // save resampling configuration as part of the worker pool configuration
        worker_pool_config.set_resampler_param(resampler_param);

        hw.set_tx_gap_samples(worker_pool_config.tx_gap_samples);

        hw.initialize_device();

        hw.initialize_buffer_tx_pool(sp3::get_N_samples_in_packet_length_max(
            worker_pool_config.maximum_packet_sizes, hw.get_samp_rate()));

        hw.initialize_buffer_rx(
            worker_pool_config.rx_ant_streams_length_slots *
            static_cast<uint32_t>(
                sp3::duration_t(hw.get_samp_rate(), sp3::duration_ec_t::slot001).get_N_samples()));

        layer_unit_vec.push_back(
            std::make_unique<worker_pool_t>(worker_pool_config, hw, phy_radio));
    }
}

}  // namespace dectnrp::phy
