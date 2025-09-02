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

#include <array>
#include <cstdint>
#include <dectnrp/constants.hpp>
#include <vector>

#include "dectnrp/common/complex.hpp"

namespace dectnrp::phy {

class channel_antenna_t {
    public:
        explicit channel_antenna_t(const uint32_t b_max_, const uint32_t N_eff_TX_max_);
        ~channel_antenna_t();

        channel_antenna_t() = delete;
        channel_antenna_t(const channel_antenna_t&) = delete;
        channel_antenna_t& operator=(const channel_antenna_t&) = delete;
        channel_antenna_t(channel_antenna_t&&) = delete;
        channel_antenna_t& operator=(channel_antenna_t&&) = delete;

        typedef std::array<uint32_t, constants::N_TS_max> write_offsets_t;

        static const write_offsets_t& get_write_offsets(const uint32_t ps_idx,
                                                        const uint32_t ofdm_symb_ps_idx) {
            // Figure 4.5-2 and 4.5-3 as reference

            // even processing stage index
            if (ps_idx % 2 == 0) {
                /* Left-side or right side symbol?
                 *
                 * Left-side can be  0 1 2 3 or 4 5 6 7.
                 * Right-side can be 2 3 0 1 or 6 7 4 5.
                 */
                return ofdm_symb_ps_idx <= 1 ? template_00110011 : template_11001100;
            }

            // odd processing stage index

            /* Left-side or right side symbol?
             *
             * Left-side can be  2 3 0 1 or 6 7 4 5.
             * Right-side can be 0 1 2 3 or 4 5 6 7.
             */
            return ofdm_symb_ps_idx <= 1 ? template_11001100 : template_00110011;
        };

        /// read-only access
        const cf_t* get_chestim_drs_zf(const std::size_t idx) const { return chestim_drs_zf[idx]; };

        /// we perform the channel estimation directly in rx_synced_t
        friend class rx_synced_t;

    private:
        /// templates for offsets
        static constexpr write_offsets_t template_00110011{0, 0, 1, 1, 0, 0, 1, 1};
        static constexpr write_offsets_t template_11001100{1, 1, 0, 0, 1, 1, 0, 0};

        /**
         * \brief One pointer of DRS cell channel estimates per OFDM symbol and transmit stream,
         * gained by zero-forcing pilots.
         *
         *  dimension:  N_eff_TX_max x (14 * b_max_)
         *            = N_eff_TX_max x sp3::drs_t::get_nof_drs_subc(b_max)
         */
        std::vector<cf_t*> chestim_drs_zf;

        /**
         * \brief One pointer of DRS cell channel estimates of two OFDM symbols per transmit stream
         * and two OFDM symbols, gained by zero-forcing pilots. DRS cells channel  from both sides
         * are interlaced, see Figure 4.5-2 and Figure 4.5-3 in part 3. Note that N_eff_TX_max =
         * N_TS_max.
         *
         *  dimension:  N_eff_TX_max x (2 * sp3::drs_t::get_nof_drs_subc(b_max))
         */
        std::vector<cf_t*> chestim_drs_zf_interlaced;

        /**
         * \brief One pointer of channel estimates for a single OFDM symbol (+ 1 for DC) for each
         * TS. Note that N_eff_TX_max = N_TS_max.
         *
         *  dimension:  N_eff_TX_max x (56 * b_max_ + 1)
         */
        std::vector<cf_t*> chestim;
};

}  // namespace dectnrp::phy
