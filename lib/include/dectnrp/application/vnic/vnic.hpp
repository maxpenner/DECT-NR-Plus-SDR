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

#include <netinet/in.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dectnrp::application::vnic {

class vnic_t {
    public:
        vnic_t() = default;
        virtual ~vnic_t() = default;

        vnic_t(const vnic_t&) = delete;
        vnic_t& operator=(const vnic_t&) = delete;
        vnic_t(vnic_t&&) = delete;
        vnic_t& operator=(vnic_t&&) = delete;

    protected:
        // ##################################################
        // IP V4

        static uint32_t get_ip4_header_length_byte(const uint8_t* ip4);
        static uint32_t get_ip4_daddr(const uint8_t* ip4);
        static std::string get_ip4_daddr_str(const uint8_t* ip4);

        // ##################################################
        // IP V6

        static constexpr uint32_t get_ip6_header_length_byte() { return 40; };
        static struct in6_addr get_ip6_daddr(const uint8_t* ip6);
        static std::string get_ip6_daddr_str(const uint8_t* ip6);

        // ##################################################
        /// IP V4 or V6

        static uint32_t get_ip_version(const uint8_t* ipx);
        static uint32_t get_ip_header_length(const uint8_t* ipx);

        // ##################################################
        // UDP
        // -

        // ##################################################
        // PTP
        // -
};

}  // namespace dectnrp::application::vnic
