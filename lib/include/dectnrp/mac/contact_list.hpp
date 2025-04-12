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
#include <vector>

#include "dectnrp/common/adt/bimap.hpp"
#include "dectnrp/mac/contact.hpp"
#include "dectnrp/phy/rx/rx_synced/mimo/mimo_csi.hpp"
#include "dectnrp/phy/rx/sync/sync_report.hpp"

namespace dectnrp::mac {

template <std::derived_from<contact_t> T>
class contact_list_t {
    public:
        void reserve(const std::size_t N_entries) {
            srdid_bimap.reserve(N_entries);
            conn_idx_server_bimap.reserve(N_entries);
            conn_idx_client_bimap.reserve(N_entries);

            contact_idx_um.reserve(N_entries);
            contacts_vec.reserve(N_entries);
        }

        void add_new_contact(const uint32_t lrdid,
                             const uint32_t srdid,
                             const uint32_t conn_idx_server,
                             const uint32_t conn_idx_client) noexcept {
            srdid_bimap.insert(lrdid, srdid);
            conn_idx_server_bimap.insert(lrdid, conn_idx_server);
            conn_idx_client_bimap.insert(lrdid, conn_idx_client);

            contact_idx_um.insert(std::make_pair(lrdid, contacts_vec.size()));
            contacts_vec.push_back(T{});
        }

        bool is_lrdid_known(const uint32_t lrdid) const noexcept {
            return srdid_bimap.is_k_known(lrdid);
        }

        bool is_srdid_known(const uint32_t srdid) const noexcept {
            return srdid_bimap.is_v_known(srdid);
        }

        uint32_t get_lrdid_from_srdid(const uint32_t srdid) const noexcept {
            return srdid_bimap.get_k(srdid);
        };

        uint32_t get_srdid_from_lrdid(const uint32_t lrdid) const noexcept {
            return srdid_bimap.get_v(lrdid);
        };

        uint32_t get_lrdid_from_conn_idx_server(const uint32_t conn_idx_server) const noexcept {
            return conn_idx_server_bimap.get_k(conn_idx_server);
        };

        uint32_t get_conn_idx_client_from_lrdid(const uint32_t lrdid) const noexcept {
            return conn_idx_client_bimap.get_v(lrdid);
        };

        constexpr const T& get_contact(const uint32_t lrdid) const& noexcept {
            return contacts_vec.at(contact_idx_um.at(lrdid));
        };

        constexpr T& get_contact(const uint32_t lrdid) & noexcept {
            return contacts_vec.at(contact_idx_um.at(lrdid));
        };

        const std::vector<T>& get_contacts_vec() const noexcept { return contacts_vec; };

    private:
        /// map various identifiers to long radio device ID
        common::adt::bimap_t<uint32_t, uint32_t, true> srdid_bimap;
        common::adt::bimap_t<uint32_t, uint32_t, true> conn_idx_server_bimap;
        common::adt::bimap_t<uint32_t, uint32_t, true> conn_idx_client_bimap;

        /// map long radio device ID to local contact index
        std::unordered_map<uint32_t, uint32_t> contact_idx_um;

        /// list of all contacts_vec accessible via the local index
        std::vector<T> contacts_vec;
};

}  // namespace dectnrp::mac
