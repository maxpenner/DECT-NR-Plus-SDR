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

#include "dectnrp/common/complex.hpp"

namespace dectnrp::simulation {

class vspp_t {
    public:
        /**
         * \brief vspp = virtual samples per packet
         *
         * https://files.ettus.com/manual/structuhd_1_1stream__args__t.html#a4463f2eec2cc7ee70f84baacbb26e1ef
         *
         * \param id_
         * \param nof_antennas_
         * \param samp_rate_
         * \param spp_size_
         */
        explicit vspp_t(const uint32_t id_,
                        const uint32_t nof_antennas_,
                        const uint32_t samp_rate_,
                        const uint32_t spp_size_);
        virtual ~vspp_t();

        vspp_t() = delete;
        vspp_t(const vspp_t&) = delete;
        vspp_t& operator=(const vspp_t&) = delete;
        vspp_t(vspp_t&&) = delete;
        vspp_t& operator=(vspp_t&&) = delete;

        std::vector<cf_t*> spp;

        void spp_zero(uint32_t offset, uint32_t nof_samples);
        void spp_zero();

        const uint32_t id;
        const uint32_t nof_antennas;
        const uint32_t samp_rate;
        const uint32_t spp_size;

    protected:
        bool are_args_valid(const uint32_t inp_dim, uint32_t offset, uint32_t nof_samples) const;
};

}  // namespace dectnrp::simulation
