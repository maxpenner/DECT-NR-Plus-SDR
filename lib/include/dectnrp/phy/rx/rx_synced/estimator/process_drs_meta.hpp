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

#include <cstddef>

namespace dectnrp::phy {

/**
 * \brief Instances of this class contain meta information about an OFDM symbol with DRS
 * cells. Each deriving class can use that information as needed.
 */
class process_drs_meta_t {
    public:
        process_drs_meta_t(const std::size_t N_TS_,
                           const std::size_t TS_idx_first_,
                           const std::size_t TS_idx_last_,
                           const std::size_t ofdm_symb_idx_)
            : N_TS(N_TS_),
              TS_idx_first(TS_idx_first_),
              TS_idx_last(TS_idx_last_),
              ofdm_symb_idx(ofdm_symb_idx_) {}

        process_drs_meta_t() = delete;
        process_drs_meta_t(const process_drs_meta_t&) = delete;
        process_drs_meta_t& operator=(const process_drs_meta_t&) = delete;
        process_drs_meta_t(process_drs_meta_t&&) = delete;
        process_drs_meta_t& operator=(process_drs_meta_t&&) = delete;

        /// number of transmit streams received
        const std::size_t N_TS;

        /// index of first transmit stream, can be 0 or 4
        const std::size_t TS_idx_first;

        /// index of last transmit stream, can be 0, 1, 3 or 7
        const std::size_t TS_idx_last;

        /// absolute OFDM symbol index, starting at 0 for the STF
        const std::size_t ofdm_symb_idx;
};

}  // namespace dectnrp::phy
