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

#include "dectnrp/phy/phy_config.hpp"

#include <cmath>

#include "dectnrp/common/json_parse.hpp"
#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/resample/resampler.hpp"

namespace dectnrp::phy {

const std::array<resampler_param_t, 28> phy_config_t::resampler_param_verified = {
    // DECTNRP rates
    resampler_param_t(constants::samp_rate_min_u_b, 1, 1),
    resampler_param_t(3456000, 1, 1),
    resampler_param_t(6912000, 1, 1),
    resampler_param_t(13824000, 1, 1),
    resampler_param_t(20736000, 1, 1),
    resampler_param_t(27648000, 1, 1),
    resampler_param_t(41472000, 1, 1),
    resampler_param_t(55296000, 1, 1),
    resampler_param_t(82944000, 1, 1),
    resampler_param_t(110592000, 1, 1),
    resampler_param_t(165888000, 1, 1),
    resampler_param_t(221184000, 1, 1),

    // DECTNRP rates with oversampling of at least 2 for highest sample rates
    resampler_param_t(331776000, 1, 1),  // 165.888 with OS = 2
    resampler_param_t(442368000, 1, 1),  // 221.184 with OS = 2

    // LTE rates (30.72 MHz)
    resampler_param_t(1920000, 10, 9),     //   1.728
    resampler_param_t(3840000, 10, 9),     //   3.456
    resampler_param_t(7680000, 10, 9),     //   6.912
    resampler_param_t(15360000, 10, 9),    //  13.824
    resampler_param_t(30720000, 40, 27),   //  20.736
    resampler_param_t(30720000, 10, 9),    //  27.648
    resampler_param_t(61440000, 40, 27),   //  41.472
    resampler_param_t(61440000, 10, 9),    //  55.296
    resampler_param_t(122880000, 40, 27),  //  82.944
    resampler_param_t(122880000, 10, 9),   // 110.592
    resampler_param_t(245760000, 40, 27),  // 165.888
    resampler_param_t(245760000, 10, 9),   // 221.184

    // USRP rates with oversampling of at least 2 for highest sample rates
    resampler_param_t(491520000, 40, 27),  // 165.888 with OS = 2
    resampler_param_t(491520000, 10, 9)};  // 221.184 with OS = 2

resampler_param_t phy_config_t::get_resampler_param_verified(
    const uint32_t hw_samp_rate,
    const uint32_t dect_samp_rate_os,
    const bool enforce_dectnrp_samp_rate_by_resampling) {
    // how can we map dect_samp_rate_os to hw_samp_rate?
    resampler_param_t ret;

    dectnrp_assert(ret.hw_samp_rate == 0 && ret.L == 0 && ret.M == 0,
                   "sample rate configuration not defaulted");

    for (const auto& elem : resampler_param_verified) {
        // first, check if the hardware sample rate is listed
        if (hw_samp_rate != elem.hw_samp_rate) {
            continue;
        }

        // if we must generate a signal at a DECT NR+ sample rate ...
        if (enforce_dectnrp_samp_rate_by_resampling) {
            // ... we double-check that the resampling is correct
            if (hw_samp_rate == phy::resampler_t::get_samp_rate_converted_with_temporary_overflow(
                                    dect_samp_rate_os, elem.L, elem.M)) {
                ret = elem;
                break;
            }

        } else {
            // otherwise we keep the hardware sample rate ...
            ret = elem;

            // ... but deactivate the resampling
            ret.L = 1;
            ret.M = 1;
            break;
        }
    }

    dectnrp_assert(ret.hw_samp_rate > 0 && ret.L > 0 && ret.M > 0,
                   "sample rate configuration not possible");

    return ret;
}

phy_config_t::phy_config_t(const std::string directory)
    : layer_config_t(directory, "phy.json") {
    // iterate over every phy configuration
    for (auto it = json_parsed.begin(); it != json_parsed.end(); ++it) {
        dectnrp_assert(it.key().starts_with(worker_pool_config_t::json_log_key),
                       "incorrect prefix for key {}",
                       it.key());

        worker_pool_config_t worker_pool_config;

        worker_pool_config.id =
            common::jsonparse::extract_id(it.key(), worker_pool_config_t::json_log_key);

        worker_pool_config.radio_device_class_string =
            common::jsonparse::read_string(it, "radio_device_class_string");
        worker_pool_config.radio_device_class =
            section3::get_radio_device_class(worker_pool_config.radio_device_class_string);
        worker_pool_config.maximum_packet_sizes =
            section3::get_maximum_packet_sizes(worker_pool_config.radio_device_class_string);

        worker_pool_config.nof_samples_interpacket_gap_max =
            common::jsonparse::read_int(it, "nof_samples_interpacket_gap_max", 0, 50);

        worker_pool_config.os_min = common::jsonparse::read_int(it, "os_min", 1, 8);

        worker_pool_config.enforce_dectnrp_samp_rate_by_resampling =
            common::jsonparse::read_bool(it, "enforce_dectnrp_samp_rate_by_resampling");

        // moodycamel requires at least 32
        worker_pool_config.nof_jobs = common::jsonparse::read_int(it, "nof_jobs", 32, 1024);

        // 12 is 5ms, 240 is 100ms
        worker_pool_config.rx_ant_streams_length_slots =
            common::jsonparse::read_int(it, "rx_ant_streams_length_slots", 12, 240);

        // 8 is half a slot, 384=16*24 are 24 slots
        worker_pool_config.rx_chunk_length_u8subslot =
            common::jsonparse::read_int(it, "rx_chunk_length_u8subslot", 8, 384);

        // 1 is a 16th of a slot, 64=16*4 is four slots
        worker_pool_config.rx_chunk_unit_length_u8subslot =
            common::jsonparse::read_int(it, "rx_chunk_unit_length_u8subslot", 1, 64);

        worker_pool_config.job_regular_period =
            common::jsonparse::read_int(it, "job_regular_period", 1, 48);

        const auto threads_core_prio_config_sync_vec_array =
            common::jsonparse::read_int_array(it, "threads_core_prio_config_sync_vec", 2, 8, 2);
        for (uint32_t i = 0; i < threads_core_prio_config_sync_vec_array.size(); i += 2) {
            // create empty config and fill
            common::threads_core_prio_config_t threads_core_prio_config;
            threads_core_prio_config.prio_offset = threads_core_prio_config_sync_vec_array[i];
            threads_core_prio_config.cpu_core = threads_core_prio_config_sync_vec_array[i + 1];

            // add new thread
            worker_pool_config.threads_core_prio_config_sync_vec.push_back(
                threads_core_prio_config);
        }

        const auto threads_core_prio_config_tx_rx_vec_array =
            common::jsonparse::read_int_array(it, "threads_core_prio_config_tx_rx_vec", 2, 8, 2);
        for (uint32_t i = 0; i < threads_core_prio_config_tx_rx_vec_array.size(); i += 2) {
            // create empty config and fill
            common::threads_core_prio_config_t threads_core_prio_config;
            threads_core_prio_config.prio_offset = threads_core_prio_config_tx_rx_vec_array[i];
            threads_core_prio_config.cpu_core = threads_core_prio_config_tx_rx_vec_array[i + 1];

            // add new thread
            worker_pool_config.threads_core_prio_config_tx_rx_vec.push_back(
                threads_core_prio_config);
        }

        worker_pool_config.chestim_mode_lr_default =
            common::jsonparse::read_bool(it, "chestim_mode_lr_default");

        worker_pool_config.chestim_mode_lr_t_stride_default =
            common::jsonparse::read_int(it, "chestim_mode_lr_t_stride_default", 1, 12);

        worker_pool_config.json_export_length =
            common::jsonparse::read_int(it, "json_export_length", 0, 10000);

        dectnrp_assert(worker_pool_config.id == layer_unit_config_vec.size(),
                       "incorrect id {}",
                       worker_pool_config.id);

        // add new config
        layer_unit_config_vec.push_back(worker_pool_config);
    }
}

}  // namespace dectnrp::phy
