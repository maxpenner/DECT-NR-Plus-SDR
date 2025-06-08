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

namespace dectnrp::limits {

// ##################################################
// RADIO and SIMULATION

/// simulation speed change
static constexpr int32_t simulation_samp_rate_speed_minimum{-1000};
static constexpr int32_t simulation_samp_rate_speed_maximum{100};

// ##################################################
// PHY

/// physical number of antennas
static constexpr uint32_t dectnrp_max_nof_antennas{8};

/// spatial streams
static constexpr uint32_t dectnrp_max_nof_n_ss{8};

/// QAM-256
static constexpr uint32_t dectnrp_max_mcs_index{9};

/// network IDs for scrambling
static constexpr uint32_t max_nof_network_id_for_scrambling{20};

/**
 * \brief We can control multiple pairs of a hw on the radio layer and a worker_pool on the PHY from
 * a single tpoint_t firmware. We limit the number of pairs supported.
 */
static constexpr uint32_t max_nof_radio_phy_pairs_one_tpoint{4};

static constexpr uint32_t max_irregular_callback_pending{8};

// ##################################################
// MAC

/// single MAC PDU
static constexpr uint32_t max_nof_higher_layer_signalling{8};
static constexpr uint32_t max_nof_user_plane_data_per_mac_pdu{8};

/// uplink or downlink resources per PT
static constexpr uint32_t max_nof_ul_resources_per_pt{8};
static constexpr uint32_t max_nof_dl_resources_per_pt{8};

/// maximum number of TX packets from MAC to PHY
static constexpr uint32_t max_nof_tx_packet_mac_to_phy{4};

/// regular callbacks are limited
static constexpr uint32_t max_callbacks{4};

// ##################################################
// Application

static constexpr uint32_t app_max_connections{32};
static constexpr uint32_t app_max_queue_datagram{256};
static constexpr uint32_t app_max_queue_datagram_byte{1600};

/// maximum number of datagrams reportable at once
static constexpr uint32_t max_queue_level_reported{8};

}  // namespace dectnrp::limits
