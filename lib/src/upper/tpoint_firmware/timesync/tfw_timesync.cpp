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

#include "dectnrp/upper/tpoint_firmware/timesync/tfw_timesync.hpp"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <sstream>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "header_only/nlohmann/json.hpp"

namespace dectnrp::upper::tfw::timesync {

const std::string tfw_timesync_t::firmware_name("timesync");

tfw_timesync_t::tfw_timesync_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    watch.reset();

    std::fill(
        measurements_ns_vec.begin(), measurements_ns_vec.end(), common::adt::UNDEFINED_EARLY_64);
}

void tfw_timesync_t::work_start_imminent(const int64_t start_time_64) {
    // time of first pulse generation
    sample_count_at_next_pps_change_64 =
        duration_lut.get_N_samples_at_next_full_second(start_time_64) +
        duration_lut.get_N_samples_from_duration(section3::duration_ec_t::ms001, 950);

    // time of first time offset measurement
    sample_count_at_next_measurement_64 =
        start_time_64 + duration_lut.get_N_samples_from_duration(section3::duration_ec_t::s001,
                                                                 measurement_start_delay_s);

    dectnrp_log_inf(
        "Time is {}. Starting PPS export. Time offset measurements will start in {} seconds.",
        common::watch_t::get_date_and_time(),
        measurement_start_delay_s);
}

phy::machigh_phy_t tfw_timesync_t::work_regular(const phy::phy_mac_reg_t& phy_mac_reg) {
    // PPS
    if (sample_count_at_next_pps_change_64 < phy_mac_reg.time_report.chunk_time_end_64) {
#ifdef TFW_TIMESYNC_EXPORT_1PPS
        // generate one pulse
        worksub_pps(buffer_rx.get_rx_time_passed(), 0);
#endif

        // time of next pulse generation
        sample_count_at_next_pps_change_64 +=
            duration_lut.get_N_samples_from_duration(section3::duration_ec_t::s001);
    }

    // measurement
    if (sample_count_at_next_measurement_64 < phy_mac_reg.time_report.chunk_time_end_64) {
        // first measurement
        if (N_measurements_cnt == 0) {
            start_ns_64 = watch.get_elapsed();
        }

        // last measurement
        if (N_measurements_cnt == (measurements_ns_vec.size() - 1)) {
            end_ns_64 = watch.get_elapsed();
        }

        // operating system time in nanoseconds
        const int64_t time_os_ns_64 = watch.get_elapsed();

        // hardware time in nanoseconds
        const int64_t time_hw_ns_64 =
            duration_lut.get_N_ns_from_samples(buffer_rx.get_rx_time_passed());

        // save measurement
        if (N_measurements_cnt < measurements_ns_vec.size()) {
            measurements_ns_vec.at(N_measurements_cnt) = time_os_ns_64 - time_hw_ns_64;
        }

        // log progress
        if (N_measurements_cnt <= measurements_ns_vec.size() &&
            N_measurements_cnt % N_measurements_log == 0) {
            dectnrp_log_inf(
                "Measurement {} of {}.", N_measurements_cnt, measurements_ns_vec.size());
        }

        ++N_measurements_cnt;

        // update time of next measurement
        sample_count_at_next_measurement_64 += duration_lut.get_N_samples_from_duration(
            section3::duration_ec_t::ms001, measurement_interval_ms);
    }

    return phy::machigh_phy_t();
}

phy::maclow_phy_t tfw_timesync_t::work_pcc([[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::maclow_phy_t();
}

phy::machigh_phy_t tfw_timesync_t::work_pdc_async(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_timesync_t::work_upper(
    [[maybe_unused]] const upper::upper_report_t& upper_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_timesync_t::work_chscan_async(
    [[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

std::vector<std::string> tfw_timesync_t::start_threads() {
    dectnrp_log_inf("start() called, press ctrl+c to stop program");
    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

std::vector<std::string> tfw_timesync_t::stop_threads() {
    dectnrp_log_inf("stop() called");

    // save all data to json file
    std::ostringstream filename;
    filename << "tfw_timesync" << std::setw(4) << std::setfill('0');

    nlohmann::ordered_json j_packet_data;

    j_packet_data["config"]["measurements_order"] = "Time Operating System - Time Hardware";
    j_packet_data["config"]["samp_rate"] = hw.get_samp_rate();
    j_packet_data["config"]["measurement_interval_ms"] = measurement_interval_ms;
    j_packet_data["config"]["N_measurements"] = N_measurements;
    j_packet_data["data"]["N_measurements_cnt"] = std::min(N_measurements_cnt, N_measurements);
    j_packet_data["data"]["start_ns_64"] = start_ns_64;
    j_packet_data["data"]["end_ns_64"] = end_ns_64;
    j_packet_data["data"]["measurements_ns_vec"] = measurements_ns_vec;

    // get mean value of all measurements
    const int64_t mean_64 = std::accumulate(measurements_ns_vec.begin(),
                                            measurements_ns_vec.end(),
                                            decltype(measurements_ns_vec)::value_type{0}) /
                            measurements_ns_vec.size();

    // remove mean from all measurements
    std::for_each(measurements_ns_vec.begin(), measurements_ns_vec.end(), [&](auto& elem) {
        elem -= mean_64;
    });

    // same measurements without the mean
    j_packet_data["data"]["measurements_ns_vec_mean_free"] = measurements_ns_vec;

    // save maximum and minimum deviation from mean
    j_packet_data["data"]["measurements_ns_vec_mean_free_max"] =
        *std::max_element(measurements_ns_vec.begin(), measurements_ns_vec.end());
    j_packet_data["data"]["measurements_ns_vec_mean_free_min"] =
        *std::min_element(measurements_ns_vec.begin(), measurements_ns_vec.end());

    // get span between first and last mean-free measurement
    const int64_t span_64 = measurements_ns_vec.back() - measurements_ns_vec.front();
    const double span_sec = static_cast<double>(span_64) / 1.0e9;

    // total measurements time in seconds
    const int64_t total_measurement_time_64 = end_ns_64 - start_ns_64;
    const double total_measurement_time_sec =
        static_cast<double>(total_measurement_time_64) / 1.0e9;

    j_packet_data["stats"]["mean_64"] = mean_64;
    j_packet_data["stats"]["mean_sec"] = static_cast<double>(mean_64) / 1.0e9;
    j_packet_data["stats"]["span_64"] = span_64;
    j_packet_data["stats"]["span_sec"] = span_sec;
    j_packet_data["stats"]["total_measurement_time_64"] = total_measurement_time_64;
    j_packet_data["stats"]["total_measurement_time_sec"] = total_measurement_time_sec;

    // relative clock error between both systems
    const double ppm = span_sec / total_measurement_time_sec * 1.0e6;

    j_packet_data["stats"]["ppm"] = ppm;

    std::ofstream out_file(filename.str());
    out_file << std::setw(4) << j_packet_data << std::endl;
    out_file.close();

    return std::vector<std::string>{{"tpoint " + firmware_name + " " + std::to_string(id)}};
}

#ifdef TFW_TIMESYNC_EXPORT_1PPS
void tfw_timesync_t::worksub_pps(const int64_t now_64, const size_t idx) {
    // called shortly before the next full second, what is the time of next full second?
    const int64_t A = duration_lut.get_N_samples_at_next_full_second(now_64);

    const radio::pulse_config_t pulse_config{
        .rising_edge_64 = A,
        .falling_edge_64 =
            A + duration_lut.get_N_samples_from_duration(section3::duration_ec_t::ms001, 250)};

    hw.schedule_pulse_tc(pulse_config);
}
#endif

}  // namespace dectnrp::upper::tfw::timesync
