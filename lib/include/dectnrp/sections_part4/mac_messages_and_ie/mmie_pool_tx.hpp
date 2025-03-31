/*
 * Copyright 2023-2024 Maxim Penner, Alexander Poets
 * Copyright 2025-2025 Maxim Penner
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

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/association_release_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/association_request_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/association_response_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/broadcast_indication_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/cluster_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/configuration_request_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/group_assignment_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/higher_layer_signalling.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/load_info_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mac_security_info_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/measurement_report_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/neighbouring_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/network_beacon_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/padding_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/radio_device_status_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/random_access_resource_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/rd_capability_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/reconfiguration_request_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/reconfiguration_response_message.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/resource_allocation_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/route_info_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/user_plane_data.hpp"

// separated as not standard-compliant
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/forward_to_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/power_target_ie.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/extensions/time_announce_ie.hpp"

namespace dectnrp::section4 {

class mmie_pool_tx_t {
    public:
        mmie_pool_tx_t();
        virtual ~mmie_pool_tx_t() = default;

        mmie_pool_tx_t(const mmie_pool_tx_t&) = delete;
        mmie_pool_tx_t& operator=(const mmie_pool_tx_t&) = delete;
        mmie_pool_tx_t(mmie_pool_tx_t&&) = delete;
        mmie_pool_tx_t& operator=(mmie_pool_tx_t&&) = delete;

        // number of MMIEs that elements are preallocated for
        std::size_t get_nof_mmie() const { return pool.size(); }

        // number of elements across all MMIEs
        std::size_t get_nof_elements_all_mmie() const {
            std::size_t cnt = 0;
            for (const auto& vec : pool) {
                cnt += vec.second.size();
            }
            return cnt;
        }

        /**
         * \brief Sets the number of elements that the pool contains for a given MMIE type.
         *
         * \param n number of elements to contain
         * \tparam T MMIE type, must be derived from mmie_t
         */
        template <std::derived_from<mmie_t> T>
        void set_nof_elements(const std::size_t n) {
            dectnrp_assert(n > 0, "each MMIE must be contained at least once in the pool");
            auto& vec = pool[typeid(T)];
            while (vec.size() < n) {
                vec.push_back(std::make_unique<T>());
            }
            while (vec.size() > n) {
                vec.pop_back();
            }
        }

        /**
         * \brief Retrieves and returns a MMIE of a given type from the pool. In case that the pool
         * holds multiple instances of the specified MMIE type, an index must be specified to
         * indicate which element is to be retrieved. By default, the first element is returned.
         *
         * \param i index of the element to be retrieved
         * \tparam T MMIE type, must be derived from mmie_t
         * \return reference to the requested MMIE
         */
        template <std::derived_from<mmie_t> T>
        T& get(const std::size_t i = 0) {
            auto& vec = pool[typeid(T)];
            auto* ptr = vec.at(i).get();
            return *static_cast<T*>(ptr);
        }

        /**
         * \brief Fill final unused bytes with paddings IEs. At the receiver, the first padding IE
         * should terminate the decoding.
         *
         * \param mac_pdu_offset
         * \param N_bytes_to_fill
         */
        void fill_with_padding_ies(uint8_t* mac_pdu_offset, const uint32_t N_bytes_to_fill);

    protected:
        /// Unordered map that represents the actual pool. The container maps a MMIE type to a
        /// vector of unique base pointers, each pointing to an instance of the indexed MMIE type.
        std::unordered_map<std::type_index, std::vector<std::unique_ptr<mmie_t>>> pool;
};

}  // namespace dectnrp::section4
