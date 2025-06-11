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

#include "dectnrp/mac/allocation/allocation_pt.hpp"
#include "dectnrp/mac/allocation/tx_opportunity.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
#include "dectnrp/upper/p2p/data/contact_p2p.hpp"
#include "dectnrp/upper/p2p/data/rd.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::p2p {

class steady_rd_t : public tpoint_t {
    public:
        steady_rd_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_, rd_t& rd_);
        virtual ~steady_rd_t() = default;

        /// same dispatcher for FT and PT, calls worksub_* functions
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override final;

        /// same dispatcher for FT and PT, calls worksub_* functions
        phy::machigh_phy_t work_pdc_async(const phy::phy_machigh_t& phy_machigh) override final;

    protected:
        rd_t& rd;

        // ##################################################
        // Radio Layer + PHY

        /// sets initial radio settings such as TX gain, RX gain and frequency
        virtual void init_radio() = 0;

        /// if running in a simulation, we set position and trajectory in the virtual space
        virtual void init_simulation_if_detected() = 0;

        // ##################################################
        // MAC Layer

        /// all PT identities have to be known at FT, individual PTs only need their own identity
        sp4::mac_architecture::identity_t init_identity_pt(const uint32_t firmware_id_);

        /// all PT allocations have to be known at FT, individual PTs only need their own allocation
        mac::allocation::allocation_pt_t init_allocation_pt(const uint32_t firmware_id_);

        /// FT and PT both generate unicast packets, however with different identities
        void init_packet_unicast(const uint32_t ShortRadioDeviceID_tx,
                                 const uint32_t ShortRadioDeviceID_rx,
                                 const uint32_t LongRadioDeviceID_tx,
                                 const uint32_t LongRadioDeviceID_rx);

#ifdef TFW_P2P_EXPORT_PPX
        /// FT and PT both generate a PPX
        void worksub_callback_ppx(const int64_t now_64, const size_t idx, int64_t& next_64);
#endif

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
                                        const sp3::packet_sizes_t& packet_sizes,
                                        phy::harq::process_tx_t& hp_tx);

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer

        virtual void init_appiface() = 0;

        // ##################################################
        // logging

        virtual void worksub_callback_log(const int64_t now_64) const = 0;
};

}  // namespace dectnrp::upper::tfw::p2p
