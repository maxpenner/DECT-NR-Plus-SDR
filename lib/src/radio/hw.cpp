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

#include "dectnrp/radio/hw.hpp"

#include <utility>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/thread/watch.hpp"
#include "dectnrp/limits.hpp"

namespace dectnrp::radio {

void hw_t::set_nof_antennas(const uint32_t nof_antennas_) {
    dectnrp_assert(0 < nof_antennas_ && nof_antennas_ <= nof_antennas_max,
                   "number of antennas must be larger 0 and smaller/equal nof_antennas_max");
    dectnrp_assert(nof_antennas_ <= limits::dectnrp_max_nof_antennas,
                   "number of antennas cannot exceed 8");
    dectnrp_assert((nof_antennas_ & (nof_antennas_ - 1)) == 0,
                   "number of antennas must be a power of 2");

    nof_antennas = nof_antennas_;

    tx_power_ant_0dBFS = common::ant_t(nof_antennas);
    rx_power_ant_0dBFS = common::ant_t(nof_antennas);
}

const common::ant_t& hw_t::set_tx_power_ant_0dBFS_uniform_tc(const float power_dBm) {
    for (size_t i = 0; i < nof_antennas; ++i) {
        set_tx_power_ant_0dBFS_tc(power_dBm, i);
    }

    return tx_power_ant_0dBFS;
}

const common::ant_t& hw_t::adjust_tx_power_ant_0dBFS_tc(const common::ant_t& adj_dB) {
    for (size_t i = 0; i < nof_antennas; ++i) {
        if (adj_dB.at(i) == 0.0f) {
            continue;
        }

        set_tx_power_ant_0dBFS_tc(tx_power_ant_0dBFS.at(i) + adj_dB.at(i), i);
    }

    return tx_power_ant_0dBFS;
}

const common::ant_t& hw_t::set_rx_power_ant_0dBFS_uniform_tc(const float power_dBm) {
    for (size_t i = 0; i < nof_antennas; ++i) {
        set_rx_power_ant_0dBFS_tc(power_dBm, i);
    }

    return rx_power_ant_0dBFS;
}

const common::ant_t& hw_t::adjust_rx_power_ant_0dBFS_tc(const common::ant_t& adj_dB) {
    for (size_t i = 0; i < nof_antennas; ++i) {
        if (adj_dB.at(i) == 0.0f) {
            continue;
        }

        set_rx_power_ant_0dBFS_tc(rx_power_ant_0dBFS.at(i) + adj_dB.at(i), i);
    }

    return rx_power_ant_0dBFS;
}

#ifdef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
void hw_t::schedule_pulse_tc(const pulse_config_t& pulse_config) {
    dectnrp_assert(pulse_config.rising_edge_64 < pulse_config.falling_edge_64, "pulse not causal");

    set_command_time(pulse_config.rising_edge_64);
    toggle_gpio_tc();
    set_command_time(pulse_config.falling_edge_64);
    toggle_gpio_tc();
}
#endif

uint32_t hw_t::get_tmin_samples(const tmin_t tmin) const {
    dectnrp_assert(tmin != tmin_t::CARDINALITY, "cardinality");
    return tmin_samples.at(std::to_underlying(tmin));
}

uint32_t hw_t::get_samples_in_us(const uint32_t us) const {
    dectnrp_assert(us <= 1000000, "should not be larger than one second");
    dectnrp_assert(0 < samp_rate, "samp_rate not initialized");

    return static_cast<uint32_t>(static_cast<int64_t>(samp_rate) * static_cast<int64_t>(us) /
                                 int64_t{1000000});
}

int64_t hw_t::pps_time_base_sec_in_one_second() const {
    int64_t full_sec{};

    switch (hw_config.pps_time_base) {
        using enum hw_config_t::pps_time_base_t;
        case zero:
            full_sec = 0;
            break;

        case TAI:
            full_sec = common::watch_t::
                           get_elapsed_since_epoch<int64_t, common::seconds, common::tai_clock>() +
                       1;
            break;
    }

    return full_sec;
}

}  // namespace dectnrp::radio
