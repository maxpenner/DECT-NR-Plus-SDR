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

namespace dectnrp::sp3::transport_block_size {

/**
 * \brief ETSI TS 103 636-3, 7.6.6
 *
 * \param N_SS
 * \param N_PDC_subc
 * \param N_bps
 * \return value of G
 */
uint32_t get_G(const uint32_t N_SS, const uint32_t N_PDC_subc, const uint32_t N_bps);

/**
 * \brief ETSI TS 103 636-3, 5.3
 *
 * \param N_SS
 * \param N_PDC_subc
 * \param N_bps
 * \param R_numerator
 * \param R_denominator
 * \return number of PDC bits fitting into packet, can be zero if no bits fit
 */
uint32_t get_N_PDC_bits(const uint32_t N_SS,
                        const uint32_t N_PDC_subc,
                        const uint32_t N_bps,
                        const uint32_t R_numerator,
                        const uint32_t R_denominator);

/**
 * \brief ETSI TS 103 636-3, 5.3
 *
 * \param N_TB_bits
 * \param N_SS
 * \param N_PDC_subc
 * \param N_bps
 * \param R_numerator
 * \param R_denominator
 * \param Z
 * \return number of bits in TB, can be zero for many configurations
 */
uint32_t get_N_TB_bits(const uint32_t N_SS,
                       const uint32_t N_PDC_subc,
                       const uint32_t N_bps,
                       const uint32_t R_numerator,
                       const uint32_t R_denominator,
                       const uint32_t Z);

}  // namespace dectnrp::sp3::transport_block_size
