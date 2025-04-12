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

#include <concepts>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common::adt {

template <typename T, std::floating_point F>
class ema_t {
    public:
        ema_t() = default;
        ema_t(const T val_, const F alpha_)
            : val(val_),
              alpha(alpha_) {
            dectnrp_assert(0.0f <= alpha, "too small");
            dectnrp_assert(alpha <= 1.0, "too large");
        };

        T update(const T val_new) {
            return val = static_cast<T>(static_cast<F>(val) * alpha +
                                        (F{1.0} - alpha) * static_cast<F>(val_new));
        }

        T get_val() const { return val; };
        void set_val(const T val_) { val = val_; };

    private:
        T val{};
        F alpha{};
};

}  // namespace dectnrp::common::adt
