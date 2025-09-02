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

#include "dectnrp/upper/tpoint.hpp"

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part3/tm_mode.hpp"

// comment to disable AGC for TX and/or RX
#define AGC_TX_PT
#define AGC_RX_PT

namespace dectnrp::upper {

tpoint_t::tpoint_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : layer_unit_t(tpoint_config_.json_log_key, tpoint_config_.id),
      tpoint_config(tpoint_config_),
      mac_lower(mac_lower_),
      hw(mac_lower.lower_ctrl_vec.at(0).hw),
      buffer_rx(mac_lower.lower_ctrl_vec.at(0).buffer_rx),
      worker_pool_config(mac_lower.lower_ctrl_vec.at(0).worker_pool_config),
      job_queue(mac_lower.lower_ctrl_vec.at(0).job_queue),
      duration_lut(mac_lower.lower_ctrl_vec.at(0).duration_lut),
      agc_tx(mac_lower.lower_ctrl_vec.at(0).agc_tx),
      agc_rx(mac_lower.lower_ctrl_vec.at(0).agc_rx),
      tx_order_id(mac_lower.lower_ctrl_vec.at(0).tx_order_id),
      tx_earliest_64(mac_lower.lower_ctrl_vec.at(0).tx_earliest_64),
      hpp(*mac_lower.lower_ctrl_vec.at(0).hpp.get()) {
    dectnrp_assert(tx_order_id == 0, "radio layer expects 0 as first transmission order");
    dectnrp_assert(tx_earliest_64 <= common::adt::UNDEFINED_EARLY_64,
                   "must be negative so first transmission is guaranteed to occur later");
};

#ifdef UPPER_TPOINT_ENABLE_WORK_PCC_ERROR
phy::machigh_phy_t tpoint_t::work_pcc_error([[maybe_unused]] const phy::phy_maclow_t& phy_maclow) {
    return phy::machigh_phy_t();
};
#endif

void tpoint_t::worksub_agc([[maybe_unused]] const phy::sync_report_t& sync_report,
                           [[maybe_unused]] const sp4::plcf_base_t& plcf_base,
                           [[maybe_unused]] const int64_t t_agc_tx_change_64,
                           [[maybe_unused]] const int64_t t_agc_rx_change_64,
                           [[maybe_unused]] const std::size_t hw_idx) {
#if defined(AGC_TX_PT) || defined(AGC_RX_PT)
    const auto [tx_power_adj_dB, rx_power_adj_dB] = worksub_agc_adj(sync_report, plcf_base, hw_idx);

    auto& hw_local = mac_lower.lower_ctrl_vec.at(hw_idx).hw;

    if (t_agc_tx_change_64 <= t_agc_rx_change_64) {
#ifdef AGC_TX_PT
        hw_local.set_command_time(t_agc_tx_change_64);
        hw_local.adjust_tx_power_ant_0dBFS_tc(tx_power_adj_dB);
#endif
#ifdef AGC_RX_PT
        hw_local.set_command_time(t_agc_rx_change_64);
        hw_local.adjust_rx_power_ant_0dBFS_tc(rx_power_adj_dB);
#endif
    } else {
#ifdef AGC_RX_PT
        hw_local.set_command_time(t_agc_rx_change_64);
        hw_local.adjust_rx_power_ant_0dBFS_tc(rx_power_adj_dB);
#endif
#ifdef AGC_TX_PT
        hw_local.set_command_time(t_agc_tx_change_64);
        hw_local.adjust_tx_power_ant_0dBFS_tc(tx_power_adj_dB);
#endif
    }
#endif
}

std::pair<common::ant_t, common::ant_t> tpoint_t::worksub_agc_adj(
    [[maybe_unused]] const phy::sync_report_t& sync_report,
    [[maybe_unused]] const sp4::plcf_base_t& plcf_base,
    [[maybe_unused]] const std::size_t hw_idx) {
    const auto& hw_local = mac_lower.lower_ctrl_vec.at(hw_idx).hw;

#ifdef AGC_TX_PT
    auto& agc_tx_local = mac_lower.lower_ctrl_vec.at(hw_idx).agc_tx;

    const auto tx_power_adj_dB =
        agc_tx_local.get_gain_step_dB(plcf_base.get_TransmitPower_dBm<float>(),
                                      hw_local.get_tx_power_ant_0dBFS(),
                                      hw_local.get_rx_power_ant_0dBFS(),
                                      sync_report.rms_array);
#else
    const float tx_power_adj_dB = 0.0f;
#endif

#ifdef AGC_RX_PT
    auto& agc_rx_local = mac_lower.lower_ctrl_vec.at(hw_idx).agc_rx;

    const auto rx_power_adj_dB =
        agc_rx_local.get_gain_step_dB(hw_local.get_rx_power_ant_0dBFS(), sync_report.rms_array);
#else
    const common::ant_t rx_power_adj_dB = common::ant_t(hw_local.get_nof_antennas());
#endif

    return std::make_pair(tx_power_adj_dB, rx_power_adj_dB);
}

phy::maclow_phy_t tpoint_t::worksub_pcc2pdc(const phy::phy_maclow_t& phy_maclow,
                                            const uint32_t PLCF_type,
                                            const uint32_t network_id,
                                            const uint32_t rv,
                                            const phy::harq::finalize_rx_t frx,
                                            const phy::maclow_phy_handle_t mph,
                                            uint32_t* process_id) {
    ++stats.rx_pcc2pdc_success;

    auto* hp_rx =
        hpp.get_process_rx(PLCF_type, network_id, worksub_psdef(phy_maclow, PLCF_type), rv, frx);

    dectnrp_assert(hp_rx != nullptr, "HARQ process RX unavailable");

    dectnrp_assert(!(frx != phy::harq::finalize_rx_t::reset_and_terminate && process_id == nullptr),
                   "process is not terminated, the process ID must be saved");

    if (process_id != nullptr) {
        *process_id = hp_rx->get_id();
    }

    return phy::maclow_phy_t(hp_rx, mph);
}

phy::maclow_phy_t tpoint_t::worksub_pcc2pdc_running(const uint32_t rv,
                                                    const phy::harq::finalize_rx_t frx,
                                                    const phy::maclow_phy_handle_t mph,
                                                    const uint32_t process_id) {
    ++stats.rx_pcc2pdc_running_success;

    auto* hp_rx = hpp.get_process_rx_running(process_id, rv, frx);

    dectnrp_assert(hp_rx != nullptr, "HARQ process RX unavailable, PHY may not finished yet");

    return phy::maclow_phy_t(hp_rx, mph);
}

sp3::packet_sizes_def_t tpoint_t::worksub_psdef(const phy::phy_maclow_t& phy_maclow,
                                                const uint32_t PLCF_type) const {
    const auto* plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(PLCF_type);

    dectnrp_assert(plcf_base != nullptr, "chosen PLCF is undefined");

    return sp3::packet_sizes_def_t{.u = phy_maclow.sync_report.u,
                                   .b = phy_maclow.sync_report.b,
                                   .PacketLengthType = plcf_base->PacketLengthType,
                                   .PacketLength = plcf_base->get_PacketLength(),
                                   .tm_mode_index = sp3::tmmode::get_equivalent_tm_mode(
                                       phy_maclow.sync_report.N_eff_TX, plcf_base->get_N_SS()),
                                   .mcs_index = plcf_base->DFMCS,
                                   .Z = worker_pool_config.maximum_packet_sizes.psdef.Z};
}

}  // namespace dectnrp::upper
