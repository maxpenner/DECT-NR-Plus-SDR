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

#include "dectnrp/upper/tpoint_firmware/p2p/tfw_p2p_base.hpp"
//

#include "dectnrp/constants.hpp"

namespace dectnrp::upper::tfw::p2p {

tfw_p2p_base_t::tfw_p2p_base_t(const tpoint_config_t& tpoint_config_, phy::mac_lower_t& mac_lower_)
    : tpoint_t(tpoint_config_, mac_lower_) {
    // ##################################################
    // Radio Layer + PHY

    cqi_lut =
        phy::indicators::cqi_lut_t(3, worker_pool_config.radio_device_class.mcs_index_min, 5.0f);

    hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

    // ##################################################
    // MAC Layer

    tpoint_t::hpp =
        std::make_unique<phy::harq::process_pool_t>(worker_pool_config.maximum_packet_sizes, 8, 8);

    identity_ft = section4::mac_architecture::identity_t(100, 10000000, 1000);

    // how often does the FT send beacons, how often does the PT expect beacons?
    const auto beacon_period = duration_lut.get_duration(section3::duration_ec_t::ms001, 10);

    // set allocation
    allocation_ft = mac::allocation::allocation_ft_t(
        &duration_lut, beacon_period, duration_lut.get_duration(section3::duration_ec_t::ms001, 2));

#ifdef TFW_P2P_EXPORT_1PPS
    ppx_pll = mac::ppx::ppx_pll_t(duration_lut.get_duration(section3::duration_ec_t::s001),
                                  duration_lut.get_duration(section3::duration_ec_t::ms001, 250),
                                  duration_lut.get_duration(section3::duration_ec_t::ms001, 100),
                                  beacon_period,
                                  duration_lut.get_duration(section3::duration_ec_t::ms001, 10));
#endif

    // ##################################################
    // DLC and Convergence Layer
    // -

    // ##################################################
    // Application Layer
    // -
};

section4::mac_architecture::identity_t tfw_p2p_base_t::init_identity_pt(
    const uint32_t firmware_id_) {
    // load identity of FT ...
    auto identity = identity_ft;

    // ... and increment long and short radio device ID by at least one
    identity.LongRadioDeviceID += 1 + firmware_id_;
    identity.ShortRadioDeviceID += 1 + firmware_id_;

    return identity;
}

mac::allocation::allocation_pt_t tfw_p2p_base_t::init_allocation_pt(const uint32_t firmware_id_) {
    // empty allocation for one specific pt
    mac::allocation::allocation_pt_t allocation_pt = mac::allocation::allocation_pt_t(
        &duration_lut,
        allocation_ft.get_beacon_period_as_duration(),
        duration_lut.get_duration(section3::duration_ec_t::turn_around_time_us),
        duration_lut.get_duration(section3::duration_ec_t::ms001, 16),
        duration_lut.get_duration(section3::duration_ec_t::ms001, 11));

    /* If firmware ID is larger than the number of PTs we want to support, we simply leave the
     * allocation empty. This way we can use the same code for different number of PTs, which is
     * particularly important when running a simulation.
     */
    if (N_pt <= firmware_id_) {
        return allocation_pt;
    }

    // gap for better TX/RX switching and synchronization (may reduce N_resources)
    const uint32_t gap = 0;

    // frame offset to fit beacon
    const uint32_t offset_beacon = 2 + gap;

    // same for every PT
    const uint32_t ul = 2;
    const uint32_t dl = 2;
    const uint32_t ul_gap = ul + gap;
    const uint32_t dl_gap = dl + gap;
    const uint32_t ul_gap_dl_gap = ul_gap + dl_gap;
    const uint32_t stride = ul_gap_dl_gap * N_pt;

#ifdef APPLICATION_INTERFACE_VNIC_OR_SOCKET
    // number of allocations per PT we can fit in a single frame
    const uint32_t N_resources = 4;
#else
    // number of allocations per PT we can fit in a single frame
    const uint32_t N_resources = 2;
#endif

    // PT specific
    const uint32_t offset = offset_beacon + firmware_id_ * ul_gap_dl_gap;

    allocation_pt.add_resource_regular(mac::allocation::direction_t::uplink,
                                       offset,
                                       ul,
                                       stride,
                                       N_resources,
                                       section3::duration_ec_t::slot001);

    allocation_pt.add_resource_regular(mac::allocation::direction_t::downlink,
                                       offset + ul_gap,
                                       dl,
                                       stride,
                                       N_resources,
                                       section3::duration_ec_t::slot001);

    return allocation_pt;
}

void tfw_p2p_base_t::init_packet_unicast(const uint32_t ShortRadioDeviceID_tx,
                                         const uint32_t ShortRadioDeviceID_rx,
                                         const uint32_t LongRadioDeviceID_tx,
                                         const uint32_t LongRadioDeviceID_rx) {
    // meta packet size
    section3::packet_sizes_def_t& psdef = plcf_mht_mch_unicast.psdef;
    psdef.u = worker_pool_config.radio_device_class.u_min;
    psdef.b = worker_pool_config.radio_device_class.b_min;
    psdef.PacketLengthType = 1;
    psdef.PacketLength = 2;
    psdef.tm_mode_index = 0;
    psdef.mcs_index = 4;
    psdef.Z = worker_pool_config.radio_device_class.Z_min;

    // meta PHY
    section3::tx_meta_t& tx_meta = plcf_mht_mch_unicast.tx_meta;
    tx_meta.optimal_scaling_DAC = false;
    tx_meta.DAC_scale = agc_tx.get_ofdm_amplitude_factor();
    tx_meta.iq_phase_rad = 0.0f;
    tx_meta.iq_phase_increment_s2s_post_resampling_rad = 0.0f;
    tx_meta.GI_percentage = 25;

    // define PLCFs
    section4::plcf_21_t& plcf_21 = plcf_mht_mch_unicast.plcf_21;
    plcf_21.HeaderFormat = 1;
    plcf_21.PacketLengthType = psdef.PacketLengthType;
    plcf_21.set_PacketLength_m1(psdef.PacketLength);
    plcf_21.ShortNetworkID = identity_ft.ShortNetworkID;
    plcf_21.TransmitterIdentity = ShortRadioDeviceID_tx;
    plcf_21.set_TransmitPower(0);
    plcf_21.DFMCS = psdef.mcs_index;
    plcf_21.ReceiverIdentity = ShortRadioDeviceID_rx;
    plcf_21.set_NumberOfSpatialStreams(1);
    plcf_21.Reserved = 0;
    plcf_21.FeedbackFormat = section4::feedback_info_f1_t::No_feedback;

    // pick one PLCF
    plcf_mht_mch_unicast.plcf_base_effective = &plcf_mht_mch_unicast.plcf_21;

    // define MAC header type
    plcf_mht_mch_unicast.mac_header_type.Version = section4::mac_header_type_t::version_ec::v00;
    plcf_mht_mch_unicast.mac_header_type.MAC_security =
        section4::mac_header_type_t::mac_security_ec::macsec_not_used;
    plcf_mht_mch_unicast.mac_header_type.MAC_header_type =
        section4::mac_header_type_t::mac_header_type_ec::mch_empty;

    // define MAC common header
    plcf_mht_mch_unicast.unicast_header.Reserved = 0;
    plcf_mht_mch_unicast.unicast_header.Reset = 1;
    plcf_mht_mch_unicast.unicast_header.Sequence_number = 0;
    plcf_mht_mch_unicast.unicast_header.Receiver_Address = LongRadioDeviceID_rx;
    plcf_mht_mch_unicast.unicast_header.Transmitter_Address = LongRadioDeviceID_tx;

    // pick the MAC common header from the MAC header type
    // plcf_mht_mch_unicast.mch_base_effective = &plcf_mht_mch_unicast.unicast_header;
    plcf_mht_mch_unicast.mch_base_effective = &plcf_mht_mch_unicast.mch_empty;
}

}  // namespace dectnrp::upper::tfw::p2p
