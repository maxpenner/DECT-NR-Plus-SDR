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

#include <cstdint>

namespace dectnrp::phy {

/// handles connecting the decoding of PCC and a PDC
enum class handle_pcc2pdc_t : uint32_t {
    th10 = 0,  // type 1, format 0
    th20,      // type 2, format 0
    th21,      // type 2, format 1
    CARDINALITY
};

/**
 * \brief The lower MAC can instruct PHY to continue with the PDC. If so, we pass along a
 * handle. Once the PDC is processed and the higher MAC is called, we can use the handle to
 * avoid mapping the short radio device ID to a long radio device ID.
 */
struct maclow_phy_handle_t {
        maclow_phy_handle_t() = default;
        maclow_phy_handle_t(const handle_pcc2pdc_t handle_pcc2pdc_, const uint32_t lrdid_)
            : handle_pcc2pdc(handle_pcc2pdc_),
              lrdid(lrdid_) {};

        handle_pcc2pdc_t handle_pcc2pdc{handle_pcc2pdc_t::CARDINALITY};
        uint32_t lrdid{};
};

}  // namespace dectnrp::phy
