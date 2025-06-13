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

#include "dectnrp/application/application_client.hpp"
#include "dectnrp/application/application_server.hpp"
#include "dectnrp/common/adt/callbacks.hpp"
#include "dectnrp/cvg/cvg.hpp"
#include "dectnrp/dlc/dlc.hpp"
#include "dectnrp/mac/allocation/allocation_ft.hpp"
#include "dectnrp/mac/pll/pll.hpp"
#include "dectnrp/phy/indicators/cqi_lut.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie_pool_tx.hpp"
#include "dectnrp/sections_part4/psdef_plcf_mac_pdu.hpp"

#define APPLICATION_INTERFACE_VNIC_OR_SOCKET

// #define TFW_P2P_EXPORT_PPX
#ifndef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
#undef TFW_P2P_EXPORT_PPX
#endif

#ifdef TFW_P2P_EXPORT_PPX
#include "dectnrp/mac/ppx/ppx.hpp"
#endif

// #define TFW_P2P_VARIABLE_MCS

// #define TFW_P2P_MIMO

namespace dectnrp::upper::tfw::p2p {

struct rd_t {
        // ##################################################
        // Radio Layer + PHY

        /// mapping of SNR to MCS
        phy::indicators::cqi_lut_t cqi_lut;

        // ##################################################
        // MAC Layer

        /// used for regular callbacks (logging, PPX generation etc.)
        common::adt::callbacks_t<void> callbacks;

        /// both FT and PT must know the FT's identity
        sp4::mac_architecture::identity_t identity_ft;

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
        /// when using a VNIC, only one PT is supported in this demo firmware
        const uint32_t N_pt{1};
#else
        /// when using sockets, two PTs are supported in this demo firmware
        const uint32_t N_pt{2};
#endif

        /// the FT's allocation defines beacon periods, and thus has to be known at FT and PT
        mac::allocation::allocation_ft_t allocation_ft;

        /// estimation of deviation between time bases
        mac::pll_t pll;

#ifdef TFW_P2P_EXPORT_PPX
        /// convert beacons beginnings to a PPX
        mac::ppx_t ppx;
#endif

        /// part 2 defines five MAC PDU types, these are generators for each type
        sp4::ppmp_data_t ppmp_data;
        sp4::ppmp_beacon_t ppmp_beacon;
        sp4::ppmp_unicast_t ppmp_unicast;
        sp4::ppmp_rd_broadcast_t ppmp_rd_broadcast;
        sp4::mmie_pool_tx_t mmie_pool_tx;

        /// each FT and PT may schedule multiple packets into the future
        static constexpr uint32_t max_simultaneous_tx_unicast{8};

        // ##################################################
        // DLC and Convergence Layer

        /// not implemented, just a dummy
        std::unique_ptr<cvg::cvg_t> cvg;
        std::unique_ptr<dlc::dlc_t> dlc;

        // ##################################################
        // Application Layer

        /// application_server receives data from external applications and feeds it into the SDR
        std::unique_ptr<application::application_server_t> application_server;

        /// application_client takes data from the SDR and sends it to external applications
        std::unique_ptr<application::application_client_t> application_client;

        // ##################################################
        // logging

        static constexpr uint32_t worksub_callback_log_period_sec{2};
};

}  // namespace dectnrp::upper::tfw::p2p
