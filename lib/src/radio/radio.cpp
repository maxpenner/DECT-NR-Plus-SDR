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

#include "dectnrp/radio/radio.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/radio/hw_usrp.hpp"

namespace dectnrp::radio {

radio_t::radio_t(const radio_config_t& radio_config_)
    : layer_t("RADIO"),
      radio_config(radio_config_) {
    const uint32_t nof_radio_hardware = radio_config.get_nof_layer_unit_config();

    dectnrp_assert(nof_radio_hardware > 0, "config vector of radio layer is empty");

    is_vspace_valid_if_so_init();

    for (uint32_t hw_id = 0; hw_id < nof_radio_hardware; ++hw_id) {
        const auto& hw_config = radio_config.get_layer_unit_config(hw_id);

        dectnrp_assert(hw_config.id == hw_id, "incorrect ID");

        if (hw_config.hw_name == hw_simulator_t::name) {
            layer_unit_vec.push_back(std::make_unique<hw_simulator_t>(hw_config, *vspace.get()));
        } else if (hw_config.hw_name == hw_usrp_t::name) {
            layer_unit_vec.push_back(std::make_unique<hw_usrp_t>(hw_config));
        } else {
            dectnrp_assert_failure("unknown radio type");
        }
    }
}

void radio_t::start_threads_of_all_layer_units() {
    for (auto& hw : layer_unit_vec) {
        hw->start_threads_and_iq_streaming();
    }
}

void radio_t::is_vspace_valid_if_so_init() {
    // number of devices (multi-devices hidden behind driver count as one device)
    const uint32_t nof_radio_hardware = radio_config.get_nof_layer_unit_config();

    // number of simulators found
    uint32_t nof_simulator = 0;

    // go over each hw ...
    for (uint32_t hw_id = 0; hw_id < nof_radio_hardware; ++hw_id) {
        const auto hw_config = radio_config.get_layer_unit_config(hw_id);

        // .. and make sure it is defined, and count simulators
        if (hw_config.hw_name == hw_usrp_t::name) {
            // nothing to do here
        } else if (hw_config.hw_name == hw_simulator_t::name) {
            ++nof_simulator;
        } else {
            dectnrp_assert_failure("unknown radio type {}", hw_config.hw_name);
        }
    }

    dectnrp_assert(!(nof_simulator != 0 && nof_simulator != nof_radio_hardware),
                   "devices not all real or virtual");

    /* At this point, all our hardware devices are either real physical devices or simulators. If
     * all devices are virtual, we have to initialize the virtual space in which the devices will
     * communicate with each other. Every simulator is later given a reference to the virtual space,
     * where it has to register once the simulator is fully configured (nof_antennas, samp_rate
     * etc.).
     */
    if (nof_simulator == nof_radio_hardware) {
        dectnrp_assert(
            limits::simulation_samp_rate_speed_minimum <= hw_config_t::sim_samp_rate_speed &&
                hw_config_t::sim_samp_rate_speed != -1 &&
                hw_config_t::sim_samp_rate_speed <= limits::simulation_samp_rate_speed_maximum,
            "samp_rate_speed out of bound");

        vspace = std::make_unique<simulation::vspace_t>(nof_radio_hardware,
                                                        hw_config_t::sim_samp_rate_speed,
                                                        hw_config_t::sim_channel_name_inter,
                                                        hw_config_t::sim_channel_name_intra,
                                                        hw_config_t::sim_noise_type);
    }
}

}  // namespace dectnrp::radio
