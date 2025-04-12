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

#include <cstdint>
#include <optional>
#include <string>

#include "dectnrp/common/randomgen.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes_def.hpp"
#include "dectnrp/sections_part3/mcs.hpp"
#include "dectnrp/sections_part3/numerologies.hpp"
#include "dectnrp/sections_part3/tm_mode.hpp"

namespace dectnrp::section3 {

struct packet_sizes_t {
        packet_sizes_def_t psdef{};

        numerologies_t numerology{};
        mcs_t mcs{};
        tmmode::tm_mode_t tm_mode{};

        uint32_t N_PACKET_symb{};
        uint32_t N_PDC_subc{};
        uint32_t G{};
        uint32_t N_PDC_bits{};
        uint32_t N_TB_bits{};
        uint32_t N_TB_byte{};

        uint32_t C{};

        uint32_t N_DF_symb{};

        uint32_t N_DRS_subc{};

        /// how large are the individual parts in time domain?
        uint32_t N_samples_OFDM_symbol{};
        uint32_t N_samples_STF{};
        uint32_t N_samples_STF_CP_only{};
        uint32_t N_samples_DF{};
        uint32_t N_samples_GI{};
        uint32_t N_samples_packet_no_GI{};
        uint32_t N_samples_packet{};
};

typedef std::optional<packet_sizes_t> packet_sizes_opt_t;

/**
 * \brief takes packet size definition and derives packet sizes
 *
 * There are quite a few packet configurations which seem well-defined by packet_sizes_def_t, but
 * aren't actually possible (number of symbols incorrect for a given number of antennas, filler bits
 * etc.). Due to the large amount of erroneous packet size configurations, we assume this is a
 * non-exceptional, expectable error which firmware can recover from. For this reason, the return
 * value should always be checked.
 *
 * \param qq
 * \return
 */
[[nodiscard]] packet_sizes_opt_t get_packet_sizes(const packet_sizes_def_t& qq);

/**
 * \brief maximum packet sizes for a radio device class
 *
 * \param radio_device_class_string
 * \return
 */
packet_sizes_t get_maximum_packet_sizes(const std::string& radio_device_class_string);

/**
 * \brief Given a packet size that includes neither oversampling nor resampling, what is the number
 * of samples at the specified sample rate?
 *
 * \param packet_sizes
 * \param samp_rate
 * \return
 */
uint32_t get_N_samples_in_packet_length(const packet_sizes_t& packet_sizes,
                                        const uint32_t samp_rate);

/**
 * \brief Given a packet size that includes neither oversampling nor resampling, what is the maximum
 * number of samples at the specified sample rate? Required for allocation of TX buffers.
 *
 * \param packet_sizes
 * \param samp_rate
 * \return
 */
uint32_t get_N_samples_in_packet_length_max(const packet_sizes_t& packet_sizes,
                                            const uint32_t samp_rate);

/**
 * \brief find a random packet configuration supported by the radio device class
 *
 * \param radio_device_class_string
 * \param randomgen
 * \return
 */
packet_sizes_t get_random_packet_sizes_within_rdc(const std::string& radio_device_class_string,
                                                  common::randomgen_t& randomgen);

}  // namespace dectnrp::section3
