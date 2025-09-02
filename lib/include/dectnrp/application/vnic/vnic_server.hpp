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

#include <net/if.h>

#include <string>

#include "dectnrp/application/application_server.hpp"
#include "dectnrp/application/vnic/vnic.hpp"

namespace dectnrp::application::vnic {

class vnic_server_t final : public application_server_t, public vnic_t {
    public:
        /**
         * \brief An instance of vnic_config_t can either describe a TUN or a TAP interface.
         *
         * To use a TUN, the value of tun_name has to be non-zero and tap_name is left empty.
         * To use a TAP, the value of tap_name has to be non-zero and tun_name is left empty.
         */
        struct vnic_config_t {
                std::string tun_name;    // TUN
                int MTU;                 // TUN
                std::string ip_address;  // TUN
                std::string netmask;     // TUN

                std::string tap_name;     // TAP
                std::string phynic_name;  // TAP
        };

        explicit vnic_server_t(const uint32_t id_,
                               const common::threads_core_prio_config_t thread_config_,
                               phy::job_queue_t& job_queue_,
                               const vnic_config_t vnic_config_,
                               const queue_size_t queue_size);
        ~vnic_server_t();

        vnic_server_t() = delete;
        vnic_server_t(const vnic_server_t&) = delete;
        vnic_server_t& operator=(const vnic_server_t&) = delete;
        vnic_server_t(vnic_server_t&&) = delete;
        vnic_server_t& operator=(vnic_server_t&&) = delete;

        uint32_t get_n_connections() const override final { return 1; }

        queue_level_t get_queue_level_nto(const uint32_t conn_idx,
                                          const uint32_t n) const override final;
        queue_level_t get_queue_level_try(const uint32_t conn_idx,
                                          const uint32_t n) const override final;

        uint32_t read_nto(const uint32_t conn_idx, uint8_t* dst) override final;
        uint32_t read_try(const uint32_t conn_idx, uint8_t* dst) override final;

        int get_tuntap_fd() const { return tuntap_fd; }

    private:
        ssize_t read_datagram(const uint32_t conn_idx) override final;

        bool filter_ingress_datagram(const uint32_t conn_idx) override final;

        /// TUN and TAP
        const vnic_config_t vnic_config;

        /// file descriptor (fd) of virtual network device used to poll, read and write datagrams
        int tuntap_fd;

        /// file descriptor (fd) of socket used to manipulate above network device
        int socket_fd;

        /// structure used for ioctls on the above socket
        struct ifreq ifr;

        /// TUN
        int tun_start();

        /// helpers for tun
        int static tun_alloc(char* dev, struct ifreq* ifr);
        int static tun_set_mtu(int s_fd, struct ifreq* ifr, const int MTU);
        int static tun_set_ip(int s_fd, struct ifreq* ifr, const std::string ip_address);
        int static tun_set_mask(int s_fd, struct ifreq* ifr, const std::string netmask);
        int static tun_set_up(int s_fd, struct ifreq* ifr);

        /// TAP
        int tap_start();
};

}  // namespace dectnrp::application::vnic
