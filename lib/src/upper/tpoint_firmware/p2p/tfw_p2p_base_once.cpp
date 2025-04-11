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
    dectnrp_assert(mac_lower.lower_ctrl_vec.size() == 1,
                   "firmware written for a single pair of physical and radio layer");

    // ##################################################
    // Radio Layer + PHY

    cqi_lut =
        phy::indicators::cqi_lut_t(4, worker_pool_config.radio_device_class.mcs_index_min, 8.0f);

    hw_simulator = dynamic_cast<radio::hw_simulator_t*>(&hw);

    // ##################################################
    // MAC Layer

    tpoint_t::hpp =
        std::make_unique<phy::harq::process_pool_t>(worker_pool_config.maximum_packet_sizes, 8, 8);

    identity_ft = section4::mac_architecture::identity_t(100, 10000000, 1000);

    // how often does the FT send beacons, how often does the PT expect beacons?
    const auto beacon_period = duration_lut.get_duration(section3::duration_ec_t::ms001, 10);

    allocation_ft = mac::allocation::allocation_ft_t(
        &duration_lut, beacon_period, duration_lut.get_duration(section3::duration_ec_t::ms001, 2));

#ifdef TFW_P2P_EXPORT_PPX
    ppx = mac::ppx::ppx_t(duration_lut.get_duration(section3::duration_ec_t::s001),
                          duration_lut.get_duration(section3::duration_ec_t::ms001, 250),
                          duration_lut.get_duration(section3::duration_ec_t::ms001, 20),
                          beacon_period,
                          duration_lut.get_duration(section3::duration_ec_t::ms001, 5));
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
    const uint32_t N_resources = 4;
#else
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
    section3::packet_sizes_def_t& psdef = ppmp_unicast.psdef;
    psdef.u = worker_pool_config.radio_device_class.u_min;
    psdef.b = worker_pool_config.radio_device_class.b_min;
    psdef.PacketLengthType = 1;
    psdef.PacketLength = 2;
#ifdef TFW_P2P_MIMO
    psdef.tm_mode_index = section3::tmmode::get_single_antenna_mode(buffer_rx.nof_antennas);
#else
    psdef.tm_mode_index = 0;
#endif
    psdef.mcs_index = cqi_lut.get_highest_mcs_possible(-1000.0f);
    psdef.Z = worker_pool_config.radio_device_class.Z_min;

    // define PLCFs
    section4::plcf_21_t& plcf_21 = ppmp_unicast.plcf_21;
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

    // pick a feedback format
    plcf_21.FeedbackFormat = 5;
    // prepare feedback format 4
    plcf_21.feedback_info_pool.feedback_info_f4.HARQ_feedback_bitmap = 0;
    plcf_21.feedback_info_pool.feedback_info_f4.MCS = psdef.mcs_index;
    // prepare feedback format 5
    plcf_21.feedback_info_pool.feedback_info_f5.HARQ_Process_number = 0;
    plcf_21.feedback_info_pool.feedback_info_f5.Transmission_feedback =
        section4::feedback_info_f1_t::transmission_feedback_t::ACK;
    plcf_21.feedback_info_pool.feedback_info_f5.MIMO_feedback =
        section4::feedback_info_f1_t::mimo_feedback_t::single_layer;
    plcf_21.feedback_info_pool.feedback_info_f5.Codebook_index = 0;

    // pick one PLCF
    ppmp_unicast.plcf_base_effective = &ppmp_unicast.plcf_21;

    // define MAC header type
    ppmp_unicast.mac_header_type.Version = section4::mac_header_type_t::version_ec::v00;
    ppmp_unicast.mac_header_type.MAC_security =
        section4::mac_header_type_t::mac_security_ec::macsec_not_used;
    ppmp_unicast.mac_header_type.MAC_header_type =
        section4::mac_header_type_t::mac_header_type_ec::mch_empty;

    // define MAC common header
    ppmp_unicast.unicast_header.Reserved = 0;
    ppmp_unicast.unicast_header.Reset = 1;
    ppmp_unicast.unicast_header.Sequence_number = 0;
    ppmp_unicast.unicast_header.Receiver_Address = LongRadioDeviceID_rx;
    ppmp_unicast.unicast_header.Transmitter_Address = LongRadioDeviceID_tx;

    // pick one MAC common header
    ppmp_unicast.mch_base_effective = &ppmp_unicast.mch_empty;
}

}  // namespace dectnrp::upper::tfw::p2p
