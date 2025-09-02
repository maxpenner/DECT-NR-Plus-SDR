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

#include <volk/volk.h>

#include <cstdint>
#include <vector>

#include "dectnrp/phy/rx/sync/movsum.hpp"

namespace dectnrp::phy {

class movsum_uw_t final : public movsum_t<lv_32fc_t> {
    public:
        movsum_uw_t() = default;
        ~movsum_uw_t() = default;

        /**
         * \brief Moving sum with unit weights (uw) for the individual positions within the sliding
         * moving window. This class has become necessary with the cover sequence introduced in
         * V1.5.1.
         *
         * \param uw_ unit weights
         * \param n_repelem_ number of repetitions of each unit weight
         */
        explicit movsum_uw_t(const std::vector<float> uw_, const uint32_t n_repelem_);

        /**
         * \brief Resum the value of the accumulator. Function should only be called if the position
         * of movsum_t::ptr is set correctly in alignment with the unit weights. This is particular
         * important if the elements were overwritten externally by requesting
         * movsum_t::get_front().
         */
        void resum() override final;

        void pop_push(const lv_32fc_t val) override final;

    private:
        /// unit weights
        std::vector<float> uw;

        /// how often are cover sequence values repeated?
        uint32_t n_repelem;

        float prefactor_push;
        float prefactor_pop;

        /// indices of sign changes in uw
        std::vector<uint32_t> uw_pos_neg_idx;
        std::vector<uint32_t> uw_neg_pos_idx;

        static std::vector<uint32_t> get_uw_changes_idx(const std::vector<float>& uw,
                                                        const uint32_t n_repelem,
                                                        const bool pos_neg);
};

}  // namespace dectnrp::phy
