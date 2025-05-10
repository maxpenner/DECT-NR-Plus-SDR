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

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "dectnrp/common/thread/threads.hpp"
#include "dectnrp/constants.hpp"
#include "dectnrp/phy/resample/resampler_param.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"
#include "dectnrp/sections_part3/radio_device_class.hpp"

namespace dectnrp::phy {

struct worker_pool_config_t {
        /// identifier used in JSON and log file
        static const std::string json_log_key;

        /// every worker_pool has a unique ID starting at 0
        uint32_t id;

        /// what is the maximum device class this thread pool supports?
        std::string radio_device_class_string;
        section3::radio_device_class_t radio_device_class;
        section3::packet_sizes_t maximum_packet_sizes;

        /**
         * \brief The PHY always provides the radio layer with signals of the same sample
         * rate, irrespective of bandwidth or subcarrier spacing of a specific DECTNRP
         * packet. Therefore, depending on the values of u and b, the actual oversampling
         * can be much larger. The minimum oversampling is used for the maximum values of u
         * and b from the radio device class definition.
         */
        uint32_t os_min;

        /**
         * \brief SDRs such as the N3XX or X410 have a limited set of fixed master clocks.
         * One of them typically is a multiple of 30.72MHz. When targeting a DECT NR+ sample
         * rate, fractional resampling is required (L=10 and M=9, L=40 and M=27), which is a
         * computationally very expensive bottleneck, limiting both the bandwidth and number
         * of antennas we can use.
         *
         * By setting this variable to true, we enforce resampling. For instance, when the
         * DECT sample rate if 6.912MHz, the hardware sample rate will be 30.72MHz/4=7.68MHz
         * with a resampling ratio of 10/9.
         *
         * By setting this variable to false, we omit resampling (i.e. L=M=1), even if the
         * hardware has picked an LTE master clock multiple of 30.72 MHz. As a consequence,
         * the transmitted DECT NR+ packets are wider in bandwidth (6.912MHz -> 7.68MHz) and
         * shorter in time domain (416.66us -> 416.66*6.912/7.68 = 335.93us). However, the
         * MAC layer can still use the same DECT NR+ time framing, for instance 10ms split
         * into 24 slots of 416.66us, only now the gaps between packets are longer because
         * packets are "warped" shorter.
         */
        bool enforce_dectnrp_samp_rate_by_resampling;

        /// number of jobs in queue postable to worker_tx_rx, typical value is 64 and more
        uint32_t nof_jobs;

        /**
         * \brief When generating individual packets from PHY to radio layer, the gap between
         * consecutive packets can be zero or very small (e.g. a few samples for timing/clock
         * correction). When that happens, the hardware should not switch from TX to RX and (almost)
         * immediately back to TX. Instead, the radio layer can detect these small gaps and fill
         * them with zeros to remain in TX mode. To detect gaps, both packets and their respective
         * buffers must be available to the hardware when the final samples of the first packet are
         * read.
         */
        uint32_t tx_gap_samples;

        /**
         * \brief The length of the RX ring buffer on radio layer in slots. 24 slots correspond to
         * 10ms. Typical values are between 24 (10ms) and 120 (50ms). The length also determines how
         * long instantaneous channel measurements can be. For instance, if 10ms are buffered, the
         * channel measurement must be shorter to avoid that samples are overwritten while measuring
         * the channel.
         */
        uint32_t rx_ant_streams_length_slots;

        /**
         * \brief How long is a chunk processed by a single instance of worker_sync_t in
         * u=8-subslots? A value of 32 corresponds to two slots. Note that the length defined by
         * rx_ant_streams_length_slots must be divisible by the number of instances of worker_sync_t
         * times the length defined by rx_chunk_length_u8subslot.
         */
        uint32_t rx_chunk_length_u8subslot;

        /**
         * \brief How long is a resampling unit in u=8-subslots? A value of 2 corresponds to
         * 416/16*2=52us. Typical values are 1, 2 and 4.
         */
        uint32_t rx_chunk_unit_length_u8subslot;

        /**
         * \brief How often do instances of worker_sync_t post regular jobs in multiples of chunks?
         * The chunk length is defined by rx_chunk_length_u8subslot. If rx_chunk_length_u8subslot=32
         * and rx_job_regular_period=1, a regular job is created every two slots. If
         * rx_job_regular_period=2, a regular job is created every 4 slots. For the maximum number
         * of regular jobs, rx_job_regular_period must be set to 1.
         */
        uint32_t rx_job_regular_period;

        /// thread configurations
        std::vector<common::threads_core_prio_config_t> threads_core_prio_config_sync_vec;
        std::vector<common::threads_core_prio_config_t> threads_core_prio_config_tx_rx_vec;

        /// rx_synced_t default configuration for channel estimation
        bool chestim_mode_lr_default;
        uint32_t chestim_mode_lr_t_stride_default;

        /**
         * \brief If set to zero, no JSON are exported. If set to positive value, it defines the
         * number of JSON that are collected before writing to disk.
         */
        uint32_t json_export_length;

        /// resampling from DECT sample rate to hardware sample rate, negotiated during
        /// runtime between PHY and radio layer
        mutable resampler_param_t resampler_param;

        uint32_t get_dect_samp_rate_max() const {
            return radio_device_class.u_min * radio_device_class.b_min *
                   constants::samp_rate_min_u_b;
        };

        uint32_t get_dect_samp_rate_max_oversampled() const {
            return get_dect_samp_rate_max() * os_min;
        };

        void set_resampler_param(const resampler_param_t resampler_param_) const {
            resampler_param = resampler_param_;
        };
};

}  // namespace dectnrp::phy
