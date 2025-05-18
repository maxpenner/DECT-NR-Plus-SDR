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

#include <optional>

#include "dectnrp/application/app_client.hpp"
#include "dectnrp/application/app_server.hpp"
#include "dectnrp/common/adt/callbacks.hpp"
#include "dectnrp/dlccl/dlccl.hpp"
#include "dectnrp/mac/allocation/allocation_ft.hpp"
#include "dectnrp/mac/allocation/allocation_pt.hpp"
#include "dectnrp/mac/allocation/tx_opportunity.hpp"
#include "dectnrp/mac/pll/pll.hpp"
#include "dectnrp/phy/indicators/cqi_lut.hpp"
#include "dectnrp/radio/hw_simulator.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie_pool_tx.hpp"
#include "dectnrp/sections_part4/psdef_plcf_mac_pdu.hpp"
#include "dectnrp/upper/tpoint.hpp"
#include "dectnrp/upper/tpoint_firmware/p2p/contact_p2p.hpp"

#define APPLICATION_INTERFACE_VNIC_OR_SOCKET

// #define TFW_P2P_EXPORT_PPX
#ifndef RADIO_HW_IMPLEMENTS_GPIO_TOGGLE
#undef TFW_P2P_EXPORT_PPX
#endif

#ifdef TFW_P2P_EXPORT_PPX
#include "dectnrp/mac/ppx/ppx.hpp"
#endif

// #define TFW_P2P_MIMO

namespace dectnrp::upper::tfw::p2p {

class tfw_p2p_base_t : public tpoint_t {
    public:
        tfw_p2p_base_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        virtual ~tfw_p2p_base_t() = default;

        // same dispatcher for FT and PT, calls worksub_* functions
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override final;

        // same dispatcher for FT and PT, calls worksub_* functions
        phy::machigh_phy_t work_pdc_async(const phy::phy_machigh_t& phy_machigh) override final;

    protected:
        // ##################################################
        // Radio Layer + PHY

        /// mapping of SNR to MCS
        phy::indicators::cqi_lut_t cqi_lut;

        /**
         * \brief Firmware can run on real hardware or in simulation. Whether we are in a simulation
         * is detected at runtime by casting to hw_simulator. That way we also get access to the
         * functions of the hw_simulator to change the position, trajectory and so forth.
         */
        radio::hw_simulator_t* hw_simulator{nullptr};

        /// sets initial radio settings such as TX gain, RX gain and frequency
        virtual void init_radio() = 0;

        /// if running in a simulation, we set position and trajectory in the virtual space
        virtual void init_simulation_if_detected() = 0;

        // ##################################################
        // MAC Layer

        /// used for regular callbacks (logging, PPX generation etc.)
        common::adt::callbacks_t<void> callbacks;

        /// both FT and PT must know the FT's identity
        section4::mac_architecture::identity_t identity_ft;

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
        /// when using a VNIC, only one PT is supported in this demo firmware
        const uint32_t N_pt{1};
#else
        /// when using sockets, two PTs are supported in this demo firmware
        const uint32_t N_pt{2};
#endif

        /// all PT identities have to be known at FT, individual PTs only need their own identity
        section4::mac_architecture::identity_t init_identity_pt(const uint32_t firmware_id_);

        /// the FT's allocation defines beacon periods, and thus has to be known at FT and PT
        mac::allocation::allocation_ft_t allocation_ft;

        /// all PT allocations have to be known at FT, individual PTs only need their own allocation
        mac::allocation::allocation_pt_t init_allocation_pt(const uint32_t firmware_id_);

        /// estimation of deviation between time bases
        mac::pll_t pll;

#ifdef TFW_P2P_EXPORT_PPX
        /// convert beacons beginnings to a PPX
        mac::ppx_t ppx;

        /// FT and PT both generate a PPX
        void worksub_callback_ppx(const int64_t now_64, const size_t idx, int64_t& next_64);
#endif

        /// part 2 defines five MAC PDU types, these are generators for each type
        section4::ppmp_data_t ppmp_data;
        section4::ppmp_beacon_t ppmp_beacon;
        section4::ppmp_unicast_t ppmp_unicast;
        section4::ppmp_rd_broadcast_t ppmp_rd_broadcast;
        section4::mmie_pool_tx_t mmie_pool_tx;

        /// FT and PT both generate unicast packets, however with different identities
        void init_packet_unicast(const uint32_t ShortRadioDeviceID_tx,
                                 const uint32_t ShortRadioDeviceID_rx,
                                 const uint32_t LongRadioDeviceID_tx,
                                 const uint32_t LongRadioDeviceID_rx);

        /**
         * \brief The routines for PCC type 1 are always called when the CRC for type 1 is correct.
         * There are then three different outcomes:
         *
         *  1. std::optional has a value with maclow_phy_t::continue_with_pdc = true.
         *     Don't evaluate PCC type 2 even when it too has a correct CRC, continue with PDC.
         *  2. std::optional has value with maclow_phy_t::continue_with_pdc = false.
         *     Don't evaluate PCC type 2 even when it too has a correct CRC, drop PDC.
         *  3. std::optional has no value
         *     Evaluate PCC type 2 if it has a correct CRC.
         *
         * Notation: worksub_pcc_<type><format> and worksub_pdc_<type><format>
         *
         * \param phy_maclow
         * \return
         */
        // clang-format off
        virtual std::optional<phy::maclow_phy_t> worksub_pcc_10(const phy::phy_maclow_t& phy_maclow) = 0;
        virtual phy::maclow_phy_t worksub_pcc_20(const phy::phy_maclow_t& phy_maclow) = 0;
        virtual phy::maclow_phy_t worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) = 0;

        virtual phy::machigh_phy_t worksub_pdc_10(const phy::phy_machigh_t& phy_machigh) = 0;
        virtual phy::machigh_phy_t worksub_pdc_20(const phy::phy_machigh_t& phy_machigh) = 0;
        virtual phy::machigh_phy_t worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) = 0;
        // clang-format on

        /// each FT and PT may schedule multiple packets into the future
        static constexpr uint32_t max_simultaneous_tx_unicast{8};
        virtual void worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) = 0;

        /// common procedure for FT and PT generating a single packet with multiple MAC PDUs
        bool worksub_tx_unicast(phy::machigh_phy_t& machigh_phy,
                                contact_p2p_t& contact_p2p,
                                const mac::allocation::tx_opportunity_t& tx_opportunity);

        /// update packet size depending on channel state information
        void worksub_tx_unicast_psdef(contact_p2p_t& contact_p2p, const int64_t expiration_64);

        /// insert latest values into the PLCF feedback info
        void worksub_tx_unicast_feedback(contact_p2p_t& contact_p2p, const int64_t expiration_64);

        /// fill buffer of HARQ process with MAC SDU
        bool worksub_tx_unicast_mac_sdu(const contact_p2p_t& contact_p2p,
                                        const application::queue_level_t& queue_level,
                                        const section3::packet_sizes_t& packet_sizes,
                                        phy::harq::process_tx_t& hp_tx);

        // ##################################################
        // DLC and Convergence Layer

        /// not implemented, just a dummy
        dlccl::dlccl_t dlccl;

        // ##################################################
        // Application Layer

        /// app_server receives data from external applications and feeds it into the SDR
        std::unique_ptr<application::app_server_t> app_server;

        /// app_client takes data from the SDR and sends it to external applications
        std::unique_ptr<application::app_client_t> app_client;

        virtual void init_appiface() = 0;

        // ##################################################
        // logging

        static constexpr uint32_t worksub_callback_log_period_sec{2};
        virtual void worksub_callback_log(const int64_t now_64) const = 0;
};

}  // namespace dectnrp::upper::tfw::p2p
