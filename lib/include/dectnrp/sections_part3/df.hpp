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
#include <vector>

#include "dectnrp/common/multidim.hpp"

namespace dectnrp::sp3 {

class df_t {
    public:
        /// df = data field
        df_t() = default;
        virtual ~df_t() = default;

        /**
         * \brief Called for every new packet.
         *
         * \param b beta
         * \param N_TS number of transmit streams
         */
        virtual void set_configuration(const uint32_t b, const uint32_t N_TS) = 0;

        /**
         * \brief Check if cells have to be inserted into the OFDM symbol with index l within a DECT
         * NR+ packet.
         *
         * \param l OFDM symbol index l
         * \return
         * \return
         */
        virtual bool is_symbol_index(const uint32_t l) = 0;
};

class df_pxc_t : public df_t {
    public:
        /// pxc = pcc or pdc
        df_pxc_t() = default;
        virtual ~df_pxc_t() = default;

        /// get indices of subcarrier for OFDM symbol index l
        virtual const std::vector<uint32_t>& get_k_i_one_symbol() const = 0;

    protected:
        /**
         * \brief Will have up to 6 rows, each containing one matrix, total size is [6][4][nof l].
         * Six values of b and up to four values of N_TS. l are the OFDM symbol indices.
         */
        common::vec3d<uint32_t> l_all_symbols;

        /**
         * \brief Will have up to 6 rows, each containing one matrix, total size is [6][4][nof
         * l][nof k_i]. Six values of b and up to four values of N_TS. k_i are indices of PCC
         * symbols starting with 0 in the range of the occupied subcarriers.
         */
        common::vec4d<uint32_t> k_i_all_symbols;
};

}  // namespace dectnrp::sp3
