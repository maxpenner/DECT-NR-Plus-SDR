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

namespace dectnrp::common::serdes {

class packing_t {
    public:
        virtual ~packing_t() = default;

        /// set all values to zero
        virtual void zero() = 0;

        /// called before pack() or after unpack()
        virtual bool is_valid() const = 0;

        /// called after is_valid(), typically called with pack()
        virtual uint32_t get_packed_size() const = 0;

        /// serialize
        virtual void pack(uint8_t* a_ptr) const = 0;

        /// deserialize
        virtual bool unpack(const uint8_t* a_ptr) = 0;

        void operator~() { zero(); }
        void operator>>(uint8_t* a_ptr) const { pack(a_ptr); }
        void operator<<(const uint8_t* a_ptr) { unpack(a_ptr); }
};

}  // namespace dectnrp::common::serdes
