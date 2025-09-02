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

#include <functional>

namespace dectnrp::common::adt {

template <typename R, typename... Args>
class callback_nodiscard_t {
    public:
        typedef std::function<R(Args...)> cb_t;

        callback_nodiscard_t(cb_t cb_)
            : cb(cb_) {};
        callback_nodiscard_t() = delete;

        [[nodiscard]] R operator()(Args... args) { return cb(std::forward<Args>(args)...); }

    private:
        cb_t cb;
};

}  // namespace dectnrp::common::adt
