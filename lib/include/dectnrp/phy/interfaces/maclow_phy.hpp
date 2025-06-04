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

#include "dectnrp/common/ant.hpp"
#include "dectnrp/phy/harq/process_rx.hpp"
#include "dectnrp/phy/interfaces/maclow_phy_handle.hpp"

namespace dectnrp::phy {

class maclow_phy_t {
    public:
        /// we ARE NOT interested in the PDC
        maclow_phy_t() = default;

        /// we ARE interested in the PDC, but the handle in irrelevant
        explicit maclow_phy_t(harq::process_rx_t* hp_rx_)
            : continue_with_pdc(true),
              hp_rx(hp_rx_) {};

        /// we ARE interested in the PDC
        explicit maclow_phy_t(harq::process_rx_t* hp_rx_,
                              const maclow_phy_handle_t maclow_phy_handle_)
            : continue_with_pdc(true),
              hp_rx(hp_rx_),
              maclow_phy_handle{maclow_phy_handle_} {};

        /**
         * \brief After decoding the PCC, we can set this member to true if the PHY should continue
         * with the PDC. Otherwise, the PHY will terminate the respective job after the PCC.
         */
        bool continue_with_pdc{false};

        // the following values are ignored if continue_with_pdc = false

        /**
         * \brief  Based on the content of the PLCF header, we assign a HARQ process for the
         * transport block to be used by PHY when decoding PDC.
         */
        harq::process_rx_t* hp_rx{nullptr};

        /**
         * \brief When creating an instance of this class, a firmware can set this handle to a
         * custom value. Once the PDC was decoded, the firmware will be called with the same
         * instance, and the same handle can then be used to call a respective function. This way we
         * don't have to re-check what the PLCF header type and header format were.
         */
        maclow_phy_handle_t maclow_phy_handle{};

        handle_pcc2pdc_t get_handle_pcc2pdc() const { return maclow_phy_handle.handle_pcc2pdc; };

        decltype(maclow_phy_handle_t::lrdid) get_handle_lrdid() const {
            return maclow_phy_handle.lrdid;
        };

        // ##################################################
        /* In addition to the above variables, we also include some variables related to the
         * hardware status. This is easiest done from the lower/higher MAC to the PHY as the MAC
         * controls the hardware. These variable may or not be set, we don't make it a mandatory.
         */
        struct hw_status_t {
                float tx_power_ant_0dBFS{};
                common::ant_t rx_power_ant_0dBFS{};
        } hw_status;
};

}  // namespace dectnrp::phy
