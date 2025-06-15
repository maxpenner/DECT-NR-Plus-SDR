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

#include "dectnrp/upper/txrxdelay/tfw_txrxdelay.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/common/prog/log.hpp"
#include "dectnrp/phy/agc/agc_tx.hpp"
#include "dectnrp/phy/interfaces/machigh_phy.hpp"
#include "dectnrp/phy/rx/sync/irregular_report.hpp"
#include "dectnrp/sections_part3/derivative/duration_ec.hpp"

namespace dectnrp::upper::tfw::txrxdelay {

const std::string tfw_txrxdelay_t::firmware_name("txrxdelay");

tfw_txrxdelay_t::tfw_txrxdelay_t(const tpoint_config_t& tpoint_config_,
                                 phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    hw.set_command_time();
    hw.set_tx_power_ant_0dBFS_tc(10.0f);
    hw.set_rx_power_ant_0dBFS_uniform_tc(-30.0f);
    hw.set_freq_tc(3830.0e6);

    psdef = {.u = worker_pool_config.radio_device_class.u_min,
             .b = worker_pool_config.radio_device_class.b_min,
             .PacketLengthType = 0,
             .PacketLength = 2,
             .tm_mode_index = 0,
             .mcs_index = 2,
             .Z = worker_pool_config.radio_device_class.Z_min};

    identity_ft = sp4::mac_architecture::identity_t(100, 444, 555);

    plcf_10.HeaderFormat = 0;
    plcf_10.PacketLengthType = psdef.PacketLengthType;
    plcf_10.set_PacketLength_m1(psdef.PacketLength);
    plcf_10.ShortNetworkID = identity_ft.ShortNetworkID;
    plcf_10.TransmitterIdentity = identity_ft.ShortRadioDeviceID;
    plcf_10.set_TransmitPower(0);
    plcf_10.Reserved = 0;
    plcf_10.DFMCS = psdef.mcs_index;
}

phy::irregular_report_t tfw_txrxdelay_t::work_start(const int64_t start_time_64) {
    next_measurement_time_64 =
        start_time_64 + duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001,
                                                                 measurement_separation_ms);

    return phy::irregular_report_t(next_measurement_time_64);
}

phy::machigh_phy_t tfw_txrxdelay_t::work_regular(
    [[maybe_unused]] const phy::regular_report_t& regular_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_txrxdelay_t::work_irregular(
    [[maybe_unused]] const phy::irregular_report_t& irregular_report) {
    dectnrp_assert(next_measurement_time_64 < buffer_rx.get_rx_time_passed(), "time out-of-order");

    phy::machigh_phy_t ret;

    next_measurement_time_64 += duration_lut.get_N_samples_from_duration(sp3::duration_ec_t::ms001,
                                                                         measurement_separation_ms);

    tx_time_last_64 = generate_packet_asap(ret);

    ret.irregular_report = phy::irregular_report_t(next_measurement_time_64);

    return ret;
}

phy::maclow_phy_t tfw_txrxdelay_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    // base pointer to extract PLCF_type=1
    const auto* plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1);

    // is this the correct PLCF type?
    if (plcf_base == nullptr) {
        return phy::maclow_phy_t();
    }

    // is this the correct header type?
    if (plcf_base->get_HeaderFormat() != 0) {
        return phy::maclow_phy_t();
    }

    // cast guaranteed to work
    const auto* plcf_10_rx =
        static_cast<const sp4::plcf_10_t*>(phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1));

    dectnrp_assert(plcf_10_rx != nullptr, "cast ill-formed");

    // is this the correct short network ID?
    if (plcf_10_rx->ShortNetworkID != identity_ft.ShortNetworkID) {
        return phy::maclow_phy_t();
    }

    // is this the correct short radio device ID?
    if (plcf_10_rx->TransmitterIdentity != identity_ft.ShortRadioDeviceID) {
        return phy::maclow_phy_t();
    }

    dectnrp_log_inf(
        "TX={}   RX={}   tx2rx_diff={}",
        tx_time_last_64,
        phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64,
        phy_maclow.sync_report.fine_peak_time_corrected_by_sto_fractional_64 - tx_time_last_64);

    return phy::maclow_phy_t();
}

phy::machigh_phy_t tfw_txrxdelay_t::work_pdc(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_txrxdelay_t::work_pdc_error(
    [[maybe_unused]] const phy::phy_machigh_t& phy_machigh) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_t tfw_txrxdelay_t::work_application(
    [[maybe_unused]] const application::application_report_t& application_report) {
    return phy::machigh_phy_t();
}

phy::machigh_phy_tx_t tfw_txrxdelay_t::work_channel([[maybe_unused]] const phy::chscan_t& chscan) {
    return phy::machigh_phy_tx_t();
}

void tfw_txrxdelay_t::work_stop() {}

int64_t tfw_txrxdelay_t::generate_packet_asap(phy::machigh_phy_t& machigh_phy) {
    // request harq process
    auto* hp_tx = hpp.get_process_tx(
        1, identity_ft.NetworkID, psdef, phy::harq::finalize_tx_t::reset_and_terminate);

    // every firmware has to decide how to deal with unavailable HARQ process
    if (hp_tx == nullptr) {
        dectnrp_log_wrn("HARQ process TX unavailable");
        return common::adt::UNDEFINED_EARLY_64;
    }

    // this is now a well-defined packet size
    const sp3::packet_sizes_t& packet_sizes = hp_tx->get_packet_sizes();

    plcf_10.pack(hp_tx->get_a_plcf());

    memset(hp_tx->get_a_tb(), 0x00, packet_sizes.N_TB_byte);

    uint32_t codebook_index = 0;

    const phy::tx_meta_t tx_meta = {.optimal_scaling_DAC = false,
                                    .DAC_scale = agc_tx.get_ofdm_amplitude_factor(),
                                    .iq_phase_rad = 0.0f,
                                    .iq_phase_increment_s2s_post_resampling_rad = 0.0f,
                                    .GI_percentage = 5};

    radio::buffer_tx_meta_t buffer_tx_meta = {
        .tx_order_id = tx_order_id,
        .tx_time_64 = std::max(
            tx_earliest_64,
            buffer_rx.get_rx_time_passed() + hw.get_tmin_samples(radio::hw_t::tmin_t::turnaround))};

    ++tx_order_id;
    tx_earliest_64 = buffer_tx_meta.tx_time_64 + sp3::get_N_samples_in_packet_length(
                                                     hp_tx->get_packet_sizes(), hw.get_samp_rate());

    machigh_phy.tx_descriptor_vec.push_back(
        phy::tx_descriptor_t(*hp_tx, codebook_index, tx_meta, buffer_tx_meta));

    return buffer_tx_meta.tx_time_64;
}

}  // namespace dectnrp::upper::tfw::txrxdelay
