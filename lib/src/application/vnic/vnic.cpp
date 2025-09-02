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

#include "dectnrp/application/vnic/vnic.hpp"

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>

#include "dectnrp/common/prog/assert.hpp"

#define ASSERT_IS_IPV4 dectnrp_assert(get_ip_version(ip4) == 4, "not ipv4")
#define ASSERT_IS_IPV6 dectnrp_assert(get_ip_version(ip6) == 6, "not ipv6")

namespace dectnrp::application::vnic {

uint32_t vnic_t::get_ip4_header_length_byte(const uint8_t* ip4) {
    ASSERT_IS_IPV4;

    const struct iphdr* ip_header = (struct iphdr*)ip4;

    dectnrp_assert(20 <= ip_header->ihl * 4, "ipv4 header too small");
    dectnrp_assert(ip_header->ihl * 4 <= 60, "ipv4 header too large");

    return ip_header->ihl * 4;
}

uint32_t vnic_t::get_ip4_addr(const uint8_t* ip4, const dir_t dir) {
    ASSERT_IS_IPV4;

    const struct iphdr* ip_header = (struct iphdr*)ip4;

    return dir == dir_t::dest ? ip_header->daddr : ip_header->saddr;
}

std::string vnic_t::get_ip4_addr_str(const uint8_t* ip4, const dir_t dir) {
    ASSERT_IS_IPV4;

    const uint32_t addr = get_ip4_addr(ip4, dir);

    std::string ret;

    for (uint32_t i = 0; i < 4; ++i) {
        ret += std::to_string((addr >> (i * 8)) & 0xFF) + ".";
    }

    return ret;
}

struct in6_addr vnic_t::get_ip6_addr(const uint8_t* ip6, const dir_t dir) {
    ASSERT_IS_IPV6;

    const struct ip6_hdr* ip_header = (struct ip6_hdr*)ip6;

    return dir == dir_t::dest ? ip_header->ip6_dst : ip_header->ip6_src;
}

std::string vnic_t::get_ip6_addr_str(const uint8_t* ip6, const dir_t dir) {
    ASSERT_IS_IPV6;

    const struct in6_addr daddr = get_ip6_addr(ip6, dir);

    std::string ret;

    for (uint32_t i = 0; i < sizeof(daddr.__in6_u); ++i) {
        ret += std::to_string(daddr.__in6_u.__u6_addr8[i]) + ".";
    }

    return ret;
}

uint32_t vnic_t::get_ip_version(const uint8_t* ipx) {
    const struct iphdr* ip_header = (struct iphdr*)ipx;
    return ip_header->version;
}

uint32_t vnic_t::get_ip_header_length(const uint8_t* ipx) {
    return get_ip_version(ipx) == 4 ? get_ip4_header_length_byte(ipx)
                                    : get_ip6_header_length_byte();
}

uint32_t vnic_t::get_udp_port(const uint8_t* ipx, const dir_t dir) {
    const struct udphdr* udphdr_ptr = (const struct udphdr*)&ipx[get_ip_header_length(ipx)];

    return dir == dir_t::dest ? udphdr_ptr->dest : udphdr_ptr->source;
}

bool vnic_t::has_ptp_payload(const uint8_t* ipx) {
    const auto udp_dport = get_udp_port(ipx, dir_t::dest);

    return udp_dport == 319 || udp_dport == 320;
}

}  // namespace dectnrp::application::vnic
