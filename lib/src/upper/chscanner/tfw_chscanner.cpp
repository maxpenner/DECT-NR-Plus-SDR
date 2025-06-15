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

#include "dectnrp/upper/chscanner/tfw_chscanner.hpp"

#include <algorithm>
#include <limits>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/phy/interfaces/machigh_phy.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"
#include "dectnrp/sections_part2/channel_arrangement.hpp"

namespace dectnrp::upper::tfw::chscanner {

const std::string tfw_chscanner_t::firmware_name("chscanner");

tfw_chscanner_t::tfw_chscanner_t(const tpoint_config_t& tpoint_config_,
                                 phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_),
      next_measurement_time_64(std::numeric_limits<int64_t>::max()) {
    // init frequencies
    for (const auto band : bands) {
        const auto acfn = sp2::get_absolute_channel_frequency_numbering(band);

        for (uint32_t n = acfn.n_min; n <= acfn.n_max; n += acfn.n_spacing) {
            freqs.push_back(sp2::get_center_frequency(acfn, n).FC);
        }
    }

    N_measurement =
        common::adt::round_integer(measurement_duration_per_frequency_ms, measurement_period_ms);

    dectnrp_assert(N_measurement > 0, "at least measurement required");

    // set frequency, TX and RX power
    hw.set_command_time();
    hw.set_tx_power_ant_0dBFS_tc(-1000.0f);
    rx_power_ant_0dBFS = hw.set_rx_power_ant_0dBFS_uniform_tc(-30.0f);
    hw.set_freq_tc(freqs[freqs_idx]);
}

phy::irregular_report_t tfw_chscanner_t::work_start(const int64_t start_time_64) {
    next_measurement_time_64 =
        start_time_64 + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001, 50);

    return phy::irregular_report_t(next_measurement_time_64);
}

phy::machigh_phy_t tfw_chscanner_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_chscanner_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    // get current time
    const int64_t now_64 = buffer_rx.get_rx_time_passed();

    dectnrp_assert(next_measurement_time_64 < now_64, "time out-of-order");

    phy::machigh_phy_t ret;

    /**
     * \brief Define channel measurement over the past 5ms. We must always measure in the past,
     * therefore the first argument must be <= now_64. Furthermore, we can at most measure as much
     * as the hardware buffers, for instance 10ms, but it is better to use a shorter duration since
     * the hardware constantly keeps overwriting the oldest samples. A 5ms measurement will
     * certainly be finished before the hardware overrides the same range of samples.
     */
    ret.chscan_opt = std::optional(phy::chscan_t(now_64, sp3::duration_ec_t::ms001, 5));

    return ret;
}

phy::maclow_phy_t tfw_chscanner_t::work_pcc([[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::machigh_phy_t tfw_chscanner_t::work_pdc(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_chscanner_t::work_pdc_error(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_chscanner_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_chscanner_t::work_channel(const phy::chscan_t& chscan) {
    // save measurement
    rms_min = std::min(rms_min, chscan.get_rms_avg());
    rms_max = std::max(rms_max, chscan.get_rms_avg());

    ++N_measurement_cnt;

    // do we keep measuring in this run?
    if (N_measurement_cnt == N_measurement) {
        // show result
        dectnrp_log_inf(
            "frequency: {}MHz | rms_min={} rms_max={}", freqs[freqs_idx] / 1.0e6, rms_min, rms_max);

        if (0.5 <= rms_max) {
            dectnrp_log_wrn("ADC range is limited to +/-1. ADC may be clipping.");
        }

        ++freqs_idx;
        if (freqs_idx == freqs.size()) {
            freqs_idx = 0;
            dectnrp_log_inf(" ");
        }

        N_measurement_cnt = 0;

        rms_min = 1.0e9;
        rms_max = -1.0e9;

        // set new hardware frequency ASAP
        hw.set_command_time();
        hw.set_freq_tc(freqs[freqs_idx]);

        next_measurement_time_64 += duration_lut.get_N_samples_from_duration(
            sp3::duration_ec_t::ms001, measurement_separation_between_frequencies_ms);
    } else {
        next_measurement_time_64 += duration_lut.get_N_samples_from_duration(
            sp3::duration_ec_t::ms001, measurement_period_ms);
    }

    phy::machigh_phy_tx_t ret;

    // schedule next irregular callback
    ret.irregular_report = phy::irregular_report_t(next_measurement_time_64);

    return ret;
}

void tfw_chscanner_t::work_stop() {}

}  // namespace dectnrp::upper::tfw::chscanner
