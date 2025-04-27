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

namespace dectnrp::common::serdes {

class testing_t {
    public:
        testing_t() = default;
        virtual ~testing_t() = default;

        virtual void testing_set_random() = 0;
        virtual bool testing_is_equal(const testing_t& rhs) const = 0;
        virtual const char* testing_name() const = 0;
};

}  // namespace dectnrp::common::serdes
