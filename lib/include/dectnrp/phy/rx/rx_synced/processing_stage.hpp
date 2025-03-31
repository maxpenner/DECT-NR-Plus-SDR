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
#include <type_traits>
#include <vector>

extern "C" {
#include "srsran/config.h"
}

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

template <typename T>
    requires(std::is_arithmetic_v<T> || std::is_same_v<T, cf_t>)
class processing_stage_t {
    public:
        explicit processing_stage_t(const uint32_t N_f_domain_max_,
                                    const uint32_t N_t_domain_max_,
                                    const uint32_t N_stage_)
            : N_f_domain_max(N_f_domain_max_),
              N_t_domain_max(N_t_domain_max_),
              N_stage(N_stage_) {
            for (uint32_t i = 0; i < N_stage; ++i) {
                stage.push_back(new T[N_f_domain_max * N_t_domain_max]);
            }
        }

        ~processing_stage_t() {
            for (auto& elem : stage) {
                delete elem;
            }
        }

        processing_stage_t() = delete;
        processing_stage_t(const processing_stage_t&) = delete;
        processing_stage_t& operator=(const processing_stage_t&) = delete;
        processing_stage_t(processing_stage_t&&) = delete;
        processing_stage_t& operator=(processing_stage_t&&) = delete;

        /// must be called for every new packet configuration
        void set_configuration(const uint32_t N_f_domain_, const uint32_t N_t_domain_) {
            dectnrp_assert(N_f_domain_ <= N_f_domain_max, "N_f_domain_ larger than maximum");
            dectnrp_assert(N_t_domain_ <= N_t_domain_max, "N_t_domain_ larger than maximum");

            N_f_domain = N_f_domain_;
            N_t_domain = N_t_domain_;
        }

        /// the following functions only return correct values after calling set_configuration()

        /// get pointer to specific symbol in specific stage
        T* get_stage_specific(const uint32_t t_idx, const uint32_t stage_idx) const {
            dectnrp_assert(t_idx < N_t_domain, "t_idx too large");
            dectnrp_assert(stage_idx < N_stage, "stage_idx too large");

            return &stage[stage_idx][t_idx * N_f_domain];
        }

        /// get pointers to specific symbol in all stages
        std::vector<T*> get_stage(const uint32_t t_idx) const {
            std::vector<T*> ret(N_stage);

            for (uint32_t i = 0; i < N_stage; ++i) {
                ret[i] = get_stage_specific(t_idx, i);
            }

            return ret;
        }

        /// get pointers to specific symbol in all stages
        void get_stage_prealloc(const uint32_t t_idx, std::vector<T*>& vec) const {
            dectnrp_assert(vec.size() == N_stage, "number of antennas not the number of stages");

            for (uint32_t i = 0; i < N_stage; ++i) {
                vec[i] = get_stage_specific(t_idx, i);
            }
        }

        /// get pointers to specific symbol in all stages for read only
        std::vector<const T*> get_stage_ro(const uint32_t t_idx) const {
            std::vector<const T*> ret(N_stage);

            for (uint32_t i = 0; i < N_stage; ++i) {
                ret[i] = get_stage_specific(t_idx, i);
            }

            return ret;
        }

        /// maximum conceivable values
        const uint32_t N_f_domain_max;
        const uint32_t N_t_domain_max;
        const uint32_t N_stage;

    private:
        /// OFDM symbols after CP removal and FFT
        std::vector<T*> stage;

        /// values for a specific packet
        uint32_t N_f_domain;
        uint32_t N_t_domain;
};

/// specialization for cf_t to use aligned memory for fast multiplications
template <>
processing_stage_t<cf_t>::processing_stage_t(const uint32_t N_f_domain_max_,
                                             const uint32_t N_t_domain_max_,
                                             const uint32_t N_stage_);

template <>
processing_stage_t<cf_t>::~processing_stage_t();

}  // namespace dectnrp::phy
