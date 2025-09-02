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

#include "dectnrp/phy/rx/rx_synced/channel_estimation/channel_antennas.hpp"
#include "dectnrp/phy/rx/rx_synced/estimator/process_drs_meta.hpp"
#include "dectnrp/phy/rx/rx_synced/estimator/process_stf_meta.hpp"

namespace dectnrp::phy {

class estimator_t {
    public:
        estimator_t() = default;
        virtual ~estimator_t() = default;

        estimator_t(const estimator_t&) = delete;
        estimator_t& operator=(const estimator_t&) = delete;
        estimator_t(estimator_t&&) = delete;
        estimator_t& operator=(estimator_t&&) = delete;

        /**
         * \brief Must be set right after receiving a new packet.
         *
         * \param b beta
         * \param N_eff_TX_ number of transmit streams used by TX
         */
        void reset(const uint32_t b, const uint32_t N_eff_TX_);

        virtual void process_stf(const channel_antennas_t& channel_antennas,
                                 const process_stf_meta_t& process_stf_meta) = 0;

        virtual void process_drs(const channel_antennas_t& channel_antennas,
                                 const process_drs_meta_t& process_drs_meta) = 0;

    protected:
        uint32_t N_b_OCC_plus_DC;
        uint32_t N_STF_cells_b;
        uint32_t N_DRS_cells_b;

        uint32_t N_eff_TX;

        virtual void reset_internal() = 0;
};

}  // namespace dectnrp::phy
