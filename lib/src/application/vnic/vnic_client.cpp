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

#include "dectnrp/application/vnic/vnic_client.hpp"

#include <unistd.h>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::application::vnic {

vnic_client_t::vnic_client_t(const uint32_t id_,
                             const common::threads_core_prio_config_t thread_config_,
                             phy::job_queue_t& job_queue_,
                             const int tuntap_fd_,
                             const queue_size_t queue_size)
    : application_client_t(id_, thread_config_, job_queue_, 1, queue_size),
      vnic_t(),
      tuntap_fd(tuntap_fd_) {}

uint32_t vnic_client_t::write_immediate(const uint32_t conn_idx,
                                        const uint8_t* inp,
                                        const uint32_t n) {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");

    const auto n_w = write(tuntap_fd, (const char*)inp, n);

    dectnrp_assert(!(n_w > 0 && n != n_w), "nof bytes not the same");

    return n_w > 0 ? n_w : 0;
}

uint32_t vnic_client_t::write_nto(const uint32_t conn_idx, const uint8_t* inp, const uint32_t n) {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");
    return queue_vec.at(conn_idx)->write_nto(inp, n);
}

uint32_t vnic_client_t::write_try(const uint32_t conn_idx, const uint8_t* inp, const uint32_t n) {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");
    return queue_vec.at(conn_idx)->write_try(inp, n);
}

bool vnic_client_t::filter_egress_datagram([[maybe_unused]] const uint32_t conn_idx) {
    // nothing to do here so far

    return true;
}

uint32_t vnic_client_t::copy_from_queue_to_localbuffer(const uint32_t conn_idx) {
    dectnrp_assert(conn_idx == 0, "VNIC has only conn_idx=0");
    return queue_vec.at(conn_idx)->read_nto(buffer_local);
}

}  // namespace dectnrp::application::vnic
