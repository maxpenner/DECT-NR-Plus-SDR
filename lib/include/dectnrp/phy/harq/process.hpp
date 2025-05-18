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

#include "dectnrp/common/thread/lockable_outer_inner.hpp"
#include "dectnrp/sections_part3/derivative/packet_sizes.hpp"

namespace dectnrp::phy::harq {

class process_t : public common::lockable_outer_inner_t {
    public:
        explicit process_t(const uint32_t id_)
            : id(id_) {}
        virtual ~process_t() = default;

        process_t() = delete;
        process_t(const process_t&) = delete;
        process_t& operator=(const process_t&) = delete;
        process_t(process_t&&) = delete;
        process_t& operator=(process_t&&) = delete;

        [[nodiscard]] uint32_t get_id() const;
        [[nodiscard]] uint32_t get_PLCF_type() const;
        [[nodiscard]] uint32_t get_network_id() const;
        [[nodiscard]] const section3::packet_sizes_t& get_packet_sizes() const;
        [[nodiscard]] uint32_t get_rv() const;

        friend class process_pool_t;

    protected:
        void reset();

        const uint32_t id;
        uint32_t PLCF_type{0};
        /**
         * \brief According to 7.6.6 from part 3, scrambling can be based on 24 MSB or 8 LSB of the
         * 32 bits network ID. 24 MSB are used when PLCF Type 2 was received. The full network ID is
         * not contained in PLCF header type 1 or 2, thus, it must be provided by lower MAC. 8 LSB
         * are used when PLCF Type 1 was received. The 8 LSB is the same as the short network ID
         * which is part of PLCF header type 1 and 2.
         */
        uint32_t network_id{0};
        section3::packet_sizes_t packet_sizes{};
        uint32_t rv{0};

        /**
         * \brief First reset all variables of the deriving class, then those of the base class.
         * That terminates the process such that it can be reacquired by the process pool.
         */
        virtual void reset_and_terminate_func() = 0;
};

}  // namespace dectnrp::phy::harq
