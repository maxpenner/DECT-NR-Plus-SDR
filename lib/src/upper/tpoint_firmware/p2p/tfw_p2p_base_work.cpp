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

#include <utility>

namespace dectnrp::upper::tfw::p2p {

phy::maclow_phy_t tfw_p2p_base_t::work_pcc(const phy::phy_maclow_t& phy_maclow) {
    ++stats.rx_pcc_success;

    const section4::plcf_base_t* plcf_base = nullptr;

    std::optional<phy::maclow_phy_t> optret;

    // test PLCF type 1
    if ((plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(1)) != nullptr) {
        switch (plcf_base->get_HeaderFormat()) {
            case 0:
                optret = worksub_pcc_10(phy_maclow);
                break;
            case 1:
                [[fallthrough]];
            case 2:
                [[fallthrough]];
            case 3:
                [[fallthrough]];
            case 4:
                [[fallthrough]];
            case 5:
                [[fallthrough]];
            case 6:
                [[fallthrough]];
            case 7:
                optret = std::nullopt;
                break;
            default:
                dectnrp_assert_failure("undefined PLCF type 1 header format");
                optret = std::nullopt;
                break;
        }
    }

    // lambda to overwrite gain settings for JSON file export
    auto inject_currect_gain_settings = [&](phy::maclow_phy_t& maclow_phy) -> void {
        maclow_phy.hw_status.tx_power_ant_0dBFS =
            agc_tx.get_power_ant_0dBFS(phy_maclow.sync_report.fine_peak_time_64);
        maclow_phy.hw_status.rx_power_ant_0dBFS =
            agc_rx.get_power_ant_0dBFS(phy_maclow.sync_report.fine_peak_time_64);
    };

    // if decoded with valid content, return without testing for type 2
    if (optret.has_value()) {
        inject_currect_gain_settings(optret.value());
        return optret.value();
    }

    phy::maclow_phy_t ret;

    // test PLCF type 2
    if (plcf_base = phy_maclow.pcc_report.plcf_decoder.get_plcf_base(2); plcf_base != nullptr) {
        switch (plcf_base->get_HeaderFormat()) {
            case 0:
                ret = worksub_pcc_20(phy_maclow);
                break;
            case 1:
                ret = worksub_pcc_21(phy_maclow);
                break;
            case 2:
                [[fallthrough]];
            case 3:
                [[fallthrough]];
            case 4:
                [[fallthrough]];
            case 5:
                [[fallthrough]];
            case 6:
                [[fallthrough]];
            case 7:
                optret = std::nullopt;
                break;
            default:
                dectnrp_assert_failure("undefined PLCF type 1 header format");
                optret = std::nullopt;
                break;
        }
    }

    inject_currect_gain_settings(ret);

    return ret;
}

phy::machigh_phy_t tfw_p2p_base_t::work_pdc_async(const phy::phy_machigh_t& phy_machigh) {
    // ignore entire PDC if CRC is incorrect
    if (!phy_machigh.pdc_report.crc_status) {
        ++stats.rx_pdc_fail;
        return phy::machigh_phy_t();
    }

    ++stats.rx_pdc_success;

    // ignore PDC content if no proper MMIE is decoded
    if (!phy_machigh.pdc_report.mac_pdu_decoder.has_any_data()) {
        return phy::machigh_phy_t();
    }

    ++stats.rx_pdc_has_mmie;

    dectnrp_assert(
        phy_machigh.maclow_phy.get_handle_pcc2pdc() != phy::handle_pcc2pdc_t::CARDINALITY,
        "handle out of range");

    // call routine based on handle given during PCC processing
    switch (phy_machigh.maclow_phy.get_handle_pcc2pdc()) {
        using enum phy::handle_pcc2pdc_t;
        case th10:
            return worksub_pdc_10(phy_machigh);
        case th20:
            return worksub_pdc_20(phy_machigh);
        case th21:
            return worksub_pdc_21(phy_machigh);
        default:
            dectnrp_assert(false, "pcc2pdc not handled");
            break;
    }

    return phy::machigh_phy_t();
}

}  // namespace dectnrp::upper::tfw::p2p
