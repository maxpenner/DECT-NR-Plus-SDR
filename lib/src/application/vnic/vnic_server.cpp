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

#include "dectnrp/application/vnic/vnic_server.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dectnrp/application/vnic/vnic_server_param.hpp"
#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::application::vnic {

vnic_server_t::vnic_server_t(const uint32_t id_,
                             const common::threads_core_prio_config_t thread_config_,
                             phy::job_queue_t& job_queue_,
                             const vnic_config_t vnic_config_,
                             const queue_size_t queue_size)
    : application_server_t(id_, thread_config_, job_queue_, 1, queue_size),
      vnic_t(),
      vnic_config(vnic_config_) {
    dectnrp_assert(!(vnic_config.tun_name.size() == 0 && vnic_config.tap_name.size() == 0),
                   "either TUN or TAP");
    dectnrp_assert(!(vnic_config.tun_name.size() > 0 && vnic_config.tap_name.size() > 0),
                   "either TUN or TAP");

    if (vnic_config.tun_name.size() > 0) {
        tun_start();
    } else if (vnic_config.tap_name.size() > 0) {
        tap_start();
    }

    // add polling structure
    pfds[0].fd = tuntap_fd;
    pfds[0].events = POLLIN;
}

vnic_server_t::~vnic_server_t() {
    dectnrp_assert(tuntap_fd >= 0, "tuntap_fd not initialized");
    dectnrp_assert(socket_fd >= 0, "socket_fd not initialized");

    // bring device down
    ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(socket_fd, SIOCSIFFLAGS, &ifr) == -1) {
        dectnrp_assert_failure("failed to bring device down {}", strerror(errno));
    }

    if (vnic_config.tun_name.size() > 0) {
        // nothing to do here thus far
    } else if (vnic_config.tap_name.size() > 0) {
        // delete bridge device
        if (ioctl(socket_fd, SIOCBRDELBR, &ifr) == -1) {
            dectnrp_assert_failure("failed to delete bridge device {}", strerror(errno));
        }
    }

    close(socket_fd);
    close(tuntap_fd);
}

queue_level_t vnic_server_t::get_queue_level_nto(const uint32_t conn_idx, const uint32_t n) const {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");
    return queue_vec.at(conn_idx)->get_queue_level_nto(n);
}

queue_level_t vnic_server_t::get_queue_level_try(const uint32_t conn_idx, const uint32_t n) const {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");
    return queue_vec.at(conn_idx)->get_queue_level_try(n);
}

uint32_t vnic_server_t::read_nto(const uint32_t conn_idx, uint8_t* dst) {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");
    return queue_vec.at(conn_idx)->read_nto(dst);
}

uint32_t vnic_server_t::read_try(const uint32_t conn_idx, uint8_t* dst) {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");
    return queue_vec.at(conn_idx)->read_try(dst);
}

ssize_t vnic_server_t::read_datagram(const uint32_t conn_idx) {
    return read(pfds[conn_idx].fd, buffer_local, sizeof(buffer_local));
}

bool vnic_server_t::filter_ingress_datagram([[maybe_unused]] const uint32_t conn_idx) {
#ifdef APPLICATION_VNIC_VNIC_SERVER_ONLY_FORWARD_IPV4
    if (get_ip_version(buffer_local) != 4) {
        return false;
    }
#endif

    return true;
}

int vnic_server_t::tun_start() {
    dectnrp_assert(50 < vnic_config.MTU && vnic_config.MTU <= 1500, "MTU out-of-range");
    dectnrp_assert(vnic_config.ip_address.size() > 0, "ip_address undefined");
    dectnrp_assert(vnic_config.netmask.size() > 0, "netmask undefined");

    // name of device
    char dev_cstr[IFNAMSIZ];
    memset(dev_cstr, 0x00, IFNAMSIZ);
    strncpy(dev_cstr, vnic_config.tun_name.c_str(), IFNAMSIZ);
    dev_cstr[IFNAMSIZ - 1] = '\0';

    // zero structure
    memset(&ifr, 0, sizeof(ifr));

    if ((tuntap_fd = tun_alloc(dev_cstr, &ifr)) <= 0) {
        dectnrp_assert_failure("tun_alloc() failed, are you running as root?");
        return -1;
    }

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        close(tuntap_fd);
        dectnrp_assert_failure("unable to open socket");
        return -1;
    }

    int err;
    if ((err = tun_set_mtu(socket_fd, &ifr, vnic_config.MTU)) < 0) {
        close(socket_fd);
        dectnrp_assert_failure("MTU not set");
        return -1;
    }

    if ((err = tun_set_ip(socket_fd, &ifr, vnic_config.ip_address)) < 0) {
        close(socket_fd);
        dectnrp_assert_failure("IP not set");
        return -1;
    }

    if ((err = tun_set_mask(socket_fd, &ifr, vnic_config.netmask)) < 0) {
        close(socket_fd);
        dectnrp_assert_failure("mask not set");
        return -1;
    }

    if ((err = tun_set_up(socket_fd, &ifr)) < 0) {
        close(socket_fd);
        dectnrp_assert_failure("tun not up");
        return -1;
    }

    return 0;
}

int vnic_server_t::tun_alloc(char* dev, struct ifreq* ifr) {
    /* open the clone device */
    int fd;
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        return fd;
    }

    /* preparation of the struct ifr, of type "struct ifreq" */
    ifr->ifr_flags = IFF_TUN | IFF_NO_PI;

#if defined(__GNUG__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

    dectnrp_assert(*dev, "empty device name");

    if (*dev) {
        strncpy(ifr->ifr_name, dev, IFNAMSIZ - 1);
    }

#if defined(__GNUG__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    int err;

    /* try to create the device */
    if ((err = ioctl(fd, TUNSETIFF, (void*)ifr)) < 0) {
        close(fd);
        return err;
    }

    dectnrp_assert(strcmp(dev, ifr->ifr_name) == 0, "TUN device name not equal");

    /* if the operation was successful, write back the name of the
     * interface to the variable "dev", so the caller can know
     * it. Note that the caller MUST reserve space in *dev (see calling
     * code below)
     */
    strcpy(dev, ifr->ifr_name);

    /* this is the special file descriptor that the caller will use to talk
     * with the virtual interface
     */
    return fd;
}

int vnic_server_t::tun_set_mtu(int s_fd, struct ifreq* ifr, const int MTU) {
    ifr->ifr_mtu = MTU;

    int err;
    if ((err = ioctl(s_fd, SIOCSIFMTU, (void*)ifr)) < 0) {
        return err;
    }

    return err;
}

int vnic_server_t::tun_set_ip(int s_fd, struct ifreq* ifr, const std::string ip_address) {
    struct sockaddr_in sai;
    memset(&sai, 0, sizeof(struct sockaddr));

    sai.sin_family = AF_INET;
    sai.sin_port = 0;
    sai.sin_addr.s_addr = inet_addr(ip_address.c_str());

    memcpy(&(ifr->ifr_addr), &sai, sizeof(struct sockaddr));

    int err;
    if ((err = ioctl(s_fd, SIOCSIFADDR, ifr)) < 0) {
        return err;
    }

    return err;
}

int vnic_server_t::tun_set_mask(int s_fd, struct ifreq* ifr, const std::string netmask) {
    struct sockaddr_in sai;
    memset(&sai, 0, sizeof(struct sockaddr));

    sai.sin_family = AF_INET;
    sai.sin_port = 0;
    sai.sin_addr.s_addr = inet_addr(netmask.c_str());

    memcpy(&(ifr->ifr_netmask), &sai, sizeof(struct sockaddr));

    int err;
    if ((err = ioctl(s_fd, SIOCSIFNETMASK, ifr)) < 0) {
        return err;
    }

    return err;
}

int vnic_server_t::tun_set_up(int s_fd, struct ifreq* ifr) {
    // get flags
    int err;
    if ((err = ioctl(s_fd, SIOCGIFFLAGS, ifr)) < 0) {
        return err;
    }

    // add flags
    ifr->ifr_flags |= IFF_UP | IFF_RUNNING;

    // set flags
    if ((err = ioctl(s_fd, SIOCSIFFLAGS, ifr)) < 0) {
        return err;
    }

    return err;
}

int vnic_server_t::tap_start() {
    dectnrp_assert(vnic_config.phynic_name.size() > 0, "phynic_name undefined");

    dectnrp_assert_failure("tap not implemented yet");

    return 0;
}

}  // namespace dectnrp::application::vnic
