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

#include "dectnrp/phy/rx/rx_synced/estimator.hpp"

namespace dectnrp::phy {

class estimator_aoa_t final : public estimator_t {
    public:
        estimator_aoa_t(const uint32_t b_max, const uint32_t N_RX_);
        ~estimator_aoa_t();

        estimator_aoa_t() = delete;
        estimator_aoa_t(const estimator_aoa_t&) = delete;
        estimator_aoa_t& operator=(const estimator_aoa_t&) = delete;
        estimator_aoa_t(estimator_aoa_t&&) = delete;
        estimator_aoa_t& operator=(estimator_aoa_t&&) = delete;

        virtual void process_stf(const channel_antennas_t& channel_antennas) override final;

        virtual void process_drs(const channel_antennas_t& channel_antennas,
                                 const process_drs_meta_t& process_drs_meta) override final;

    private:
        virtual void reset_internal() override final;

        /// number of physical antennas
        const uint32_t N_RX;
};

}  // namespace dectnrp::phy
