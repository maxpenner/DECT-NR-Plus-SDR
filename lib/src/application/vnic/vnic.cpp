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

#include "dectnrp/application/vnic/vnic.hpp"

#include <netinet/ip.h>
#include <netinet/ip6.h>

namespace dectnrp::application::vnic {

uint32_t vnic_t::get_ip_version(const uint8_t* ip) {
    const struct iphdr* ip_header = (struct iphdr*)ip;
    return ip_header->version;
}

uint32_t vnic_t::get_ip4_daddr(const uint8_t* ip4) {
    const struct iphdr* ip_header = (struct iphdr*)ip4;
    return ip_header->daddr;
}

std::string vnic_t::get_ip4_addr(const uint8_t* ip4) {
    const uint32_t addr = get_ip4_daddr(ip4);

    std::string ret;

    for (uint32_t i = 0; i < 4; ++i) {
        ret += std::to_string((addr >> (i * 8)) & 0xFF) + ".";
    }

    return ret;
}

struct in6_addr vnic_t::get_ip6_daddr(const uint8_t* ip6) {
    const struct ip6_hdr* ip_header = (struct ip6_hdr*)ip6;
    return ip_header->ip6_dst;
}

std::string vnic_t::get_ip6_addr(const uint8_t* ip6) {
    const struct in6_addr addr = get_ip6_daddr(ip6);

    std::string ret;

    for (uint32_t i = 0; i < sizeof(addr.__in6_u); ++i) {
        ret += std::to_string(addr.__in6_u.__u6_addr8[i]) + ".";
    }

    return ret;
}

}  // namespace dectnrp::application::vnic
