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

#include "dectnrp/common/adt/miscellaneous.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::section4::extensions {

class time_announce_ie_t final : public mmie_packing_t {
    public:
        time_announce_ie_t();

        enum class time_type_t : uint32_t {
            not_defined = common::adt::UNDEFINED_NUMERIC_32,
            LOCAL = 0,
            TAI,
            GPS,
            upper
        };

        void set_time(const time_type_t time_type_,
                      const int32_t N_frames_until_full_sec_,
                      const int64_t full_sec_);

    private:
        void zero() override;
        bool is_valid() const override;
        uint32_t get_packed_size() const override;
        void pack(uint8_t* mac_pdu_offset) const override;
        bool unpack(const uint8_t* mac_pdu_offset) override;

        time_type_t time_type{time_type_t::not_defined};

        /// if set to 0, packet start coincided with full seconds start
        uint32_t N_frames_until_full_sec{common::adt::UNDEFINED_NUMERIC_32};

        /// elapsed seconds since epoch for given time type
        int64_t full_sec{-1};

        /// in 2017, the offset was 37 seconds
        uint32_t tai_minus_utc_seconds{common::adt::UNDEFINED_NUMERIC_32};
};

}  // namespace dectnrp::section4::extensions
