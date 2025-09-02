/*
 * Copyright 2023-present Maxim Penner
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

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/limits.hpp"
#include "dectnrp/sections_part4/mac_architecture/identity.hpp"
#include "dectnrp/upper/tpoint.hpp"

namespace dectnrp::upper::tfw::txrxagc {

class tfw_txrxagc_t final : public tpoint_t {
    public:
        explicit tfw_txrxagc_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_);
        ~tfw_txrxagc_t() = default;

        tfw_txrxagc_t() = delete;
        tfw_txrxagc_t(const tfw_txrxagc_t&) = delete;
        tfw_txrxagc_t& operator=(const tfw_txrxagc_t&) = delete;
        tfw_txrxagc_t(tfw_txrxagc_t&&) = delete;
        tfw_txrxagc_t& operator=(tfw_txrxagc_t&&) = delete;

        static const std::string firmware_name;

        phy::irregular_report_t work_start(const int64_t start_time_64) override;
        phy::machigh_phy_t work_regular(const phy::regular_report_t& regular_report) override;
        phy::machigh_phy_t work_irregular(
            const phy::irregular_report_t& irregular_report) override final;
        phy::maclow_phy_t work_pcc(const phy::phy_maclow_t& phy_maclow) override;
        phy::machigh_phy_t work_pdc(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_pdc_error(const phy::phy_machigh_t& phy_machigh) override;
        phy::machigh_phy_t work_application(
            const application::application_report_t& application_report) override;
        phy::machigh_phy_tx_t work_channel(const phy::chscan_t& chscan) override;
        void work_stop() override final;

    private:
        static constexpr int64_t measurement_spacing_ms{2000};
        int64_t measurement_time_64{0};
        int64_t measurement_cnt_64{0};

        /// packet dimensions
        sp3::packet_sizes_def_t psdef;

        /// packet spacing and measured times
        const int64_t P0_u8_subslots{0};
        const int64_t P1_u8_subslots{16 * 2};
        int64_t S0_64{common::adt::UNDEFINED_EARLY_64};
        int64_t S1_64{common::adt::UNDEFINED_EARLY_64};
        int64_t L0_64{common::adt::UNDEFINED_EARLY_64};
        int64_t L1_64{common::adt::UNDEFINED_EARLY_64};
        int64_t R0_64{common::adt::UNDEFINED_EARLY_64};
        int64_t R1_64{common::adt::UNDEFINED_EARLY_64};
        int64_t S0_old_64{common::adt::UNDEFINED_EARLY_64};
        int64_t R0_old_64{common::adt::UNDEFINED_EARLY_64};

        /// AGC settings
        static constexpr uint32_t irregular_report_agc_callback_handle{1};
        static constexpr float agc_change_dB{10.0f};
        static constexpr std::size_t nof_antennas_simultaneous{1};
        enum class agc_type_t {
            none,
            tx,
            rx,
            both
        } agc_type{agc_type_t::rx};
        enum class agc_timing_t {
            front,
            transmission_free_period
        } agc_timing{agc_timing_t::transmission_free_period};

        /// used to identify packets in RX path
        sp4::mac_architecture::identity_t identity_front;
        sp4::mac_architecture::identity_t identity_back;

        /// PLCF fixed to type 1 and header format 0
        sp4::plcf_10_t plcf_10_front;
        sp4::plcf_10_t plcf_10_back;

        std::pair<std::optional<common::ant_t>, std::optional<common::ant_t>> get_agc_adj();
        void generate_back(phy::machigh_phy_t& machigh_phy);

        /// returns packet size in samples at hw sample rate
        [[nodiscard]] int64_t generate_packet_asap(
            phy::machigh_phy_t& machigh_phy,
            const sp4::plcf_10_t& plcf_10,
            const int64_t tx_time_64,
            const std::optional<common::ant_t>& tx_power_adj_dB,
            const std::optional<common::ant_t>& rx_power_adj_dB);
};

}  // namespace dectnrp::upper::tfw::txrxagc
