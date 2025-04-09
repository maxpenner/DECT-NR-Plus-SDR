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

#include "dectnrp/mac/contact_list.hpp"
#include "dectnrp/upper/tpoint_firmware/p2p/contact_p2p.hpp"
#include "dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_base.hpp"

namespace dectnrp::upper::tfw::p2p {

class tfw_p2p_ft_t final : public tfw_p2p_base_t {
    public:
        explicit tfw_p2p_ft_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_p2p_ft_t() = default;

        tfw_p2p_ft_t() = delete;
        tfw_p2p_ft_t(const tfw_p2p_ft_t&) = delete;
        tfw_p2p_ft_t& operator=(const tfw_p2p_ft_t&) = delete;
        tfw_p2p_ft_t(tfw_p2p_ft_t&&) = delete;
        tfw_p2p_ft_t& operator=(tfw_p2p_ft_t&&) = delete;

        static const std::string firmware_name;

        void work_start_imminent(const int64_t start_time_64) override;
        phy::machigh_phy_t work_regular(const phy::phy_mac_reg_t& phy_mac_reg) override;
        // phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override;
        // phy::machigh_phy_t work_pdc_async(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_upper(const upper::upper_report_t& upper_report) override;
        phy::machigh_phy_tx_t work_chscan_async(const phy::chscan_t& chscan) override;

    private:
        std::vector<std::string> start_threads() override final;
        std::vector<std::string> stop_threads() override final;

        // ##################################################
        // Radio Layer + PHY

        /// FT uses a fixed transmit power that must be written into the PLCF
        float TransmitPower_dBm_fixed;

        void init_radio() override final;
        void init_simulation_if_detected() override final;

#ifdef TFW_P2P_EXPORT_1PPS
        void worksub_callback_pps(const int64_t now_64,
                                  const size_t idx,
                                  int64_t& next_64) override final;
#endif

        // ##################################################
        // MAC Layer

        /// fast lookup of all PTs and their properties the FT requires for uplink and downlink
        mac::contact_list_t<contact_p2p_t> contact_list;

        // besides unicast for downlink, the FT also requires a beacon packet
        void init_packet_beacon();

        // clang-format off
        std::optional<phy::maclow_phy_t> worksub_pcc_10(const phy::phy_maclow_t& phy_maclow) override final;
        phy::maclow_phy_t worksub_pcc_20(const phy::phy_maclow_t& phy_maclow) override final;
        phy::maclow_phy_t worksub_pcc_21(const phy::phy_maclow_t& phy_maclow) override final;
        
        phy::machigh_phy_t worksub_pdc_10(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t worksub_pdc_20(const phy::phy_machigh_t& phy_machigh) override final;
        phy::machigh_phy_t worksub_pdc_21(const phy::phy_machigh_t& phy_machigh) override final;
        // clang-format on

        bool worksub_tx_beacon(phy::machigh_phy_t& machigh_phy);

        void worksub_tx_unicast_consecutive(phy::machigh_phy_t& machigh_phy) override final;

        // ##################################################
        // DLC and Convergence Layer
        // -

        // ##################################################
        // Application Layer

        void init_appiface() override final;

        // ##################################################
        // logging

        void worksub_callback_log(const int64_t now_64) const override final;
};

}  // namespace dectnrp::upper::tfw::p2p
