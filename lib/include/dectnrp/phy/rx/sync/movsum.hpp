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

#include <volk/volk.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace dectnrp::phy {

template <typename T>
    requires(std::is_same_v<T, float> || std::is_same_v<T, lv_32fc_t>)
class movsum_t {
    public:
        movsum_t() = default;
        virtual ~movsum_t() = default;

        explicit movsum_t(const uint32_t length_)
            : length(length_) {
            shiftreg.resize(length);
            reset();
        };

        void reset() {
            std::fill(shiftreg.begin(), shiftreg.end(), T{0});
            reset_ptr();
            sum = T{0};
        }

        void reset_ptr() { ptr = 0; };

        T get_sum() const { return sum; }
        T get_sum_front(const size_t n) const {
            T ret{0};
            for (size_t i = 0; i < n; ++i) {
                ret += shiftreg[get_front_idx(i)];
            }
            return ret;
        };
        T get_sum_back(const size_t n) const {
            T ret{0};
            for (size_t i = 0; i < n; ++i) {
                ret += shiftreg[get_back_idx(i)];
            }
            return ret;
        };

        virtual void resum() {
            if constexpr (std::is_same_v<T, float>) {
                volk_32f_accumulator_s32f(&sum, &shiftreg[0], length);
            } else {
                volk_32fc_accumulator_s32fc(&sum, &shiftreg[0], length);
            }
        }

        T get_mean() const { return sum / static_cast<T>(length); }

        virtual void pop_push(const T val) {
            sum -= shiftreg[ptr];
            sum += val;
            shiftreg[ptr] = val;
            ++ptr;
            ptr %= length;
        }

        /// to directly fill the contiguous vector memory with SIMD optimized instructions
        T* get_front() { return &shiftreg[0]; }

        uint32_t get_length() const { return length; };

    protected:
        std::vector<T> shiftreg;
        uint32_t length;
        uint32_t ptr;
        T sum;

        inline uint32_t get_front_idx(const size_t n_backward) const {
            const size_t nbw = n_backward % length;
            return (nbw > ptr) ? length - (nbw - ptr) : ptr - nbw;
        }

        inline uint32_t get_back_idx(const size_t n_forward) const {
            return (ptr + 1 + n_forward) % length;
        }
};

}  // namespace dectnrp::phy
