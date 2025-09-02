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

#include <algorithm>
#include <iterator>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "dectnrp/common/prog/assert.hpp"
#include "dectnrp/sections_part4/mac_messages_and_ie/mmie.hpp"

namespace dectnrp::sp4 {

class mmie_pool_tx_t {
    public:
        mmie_pool_tx_t();
        virtual ~mmie_pool_tx_t() = default;

        mmie_pool_tx_t(const mmie_pool_tx_t&) = delete;
        mmie_pool_tx_t& operator=(const mmie_pool_tx_t&) = delete;
        mmie_pool_tx_t(mmie_pool_tx_t&&) = delete;
        mmie_pool_tx_t& operator=(mmie_pool_tx_t&&) = delete;

        /// number of different types of MMIEs in the pool
        [[nodiscard]] std::size_t get_nof_mmie() const noexcept { return pool.size(); }

        /// number of different types of MMIEs in the pool derived from T
        template <typename T>
        [[nodiscard]] std::size_t get_nof_mmie_derived_from() const noexcept {
            return std::count_if(pool.begin(), pool.end(), [](const auto& elem) {
                return dynamic_cast<const T*>(elem.second.at(0).get()) != nullptr;
            });
        }

        /// number of elements across all MMIEs
        [[nodiscard]] std::size_t get_nof_mmie_elements() const noexcept {
            std::size_t cnt = 0;
            for (const auto& vec : pool) {
                cnt += vec.second.size();
            }
            return cnt;
        }

        /// get number of elements of a specific MMIE
        template <std::derived_from<mmie_t> T>
        [[nodiscard]] std::size_t get_nof_elements() noexcept {
            return pool[typeid(T)].size();
        }

        /// set number of elements of a specific MMIE
        template <std::derived_from<mmie_t> T>
        void set_nof_elements(const std::size_t n) noexcept {
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
         * indicate which element is to be retrieved.
         *
         * \param i index of the element to be retrieved
         * \tparam T MMIE type, must be derived from mmie_t
         * \return reference to the requested MMIE
         */
        template <std::derived_from<mmie_t> T>
            requires(std::derived_from<T, mu_depending_t> == false)
        [[nodiscard]] T& get(const std::size_t i) noexcept {
            auto& vec = pool[typeid(T)];
            auto* ptr = vec.at(i).get();
            return *static_cast<T*>(ptr);
        }

        /// same as above, but in case the class depends on mu, it must be passed as an argument
        template <std::derived_from<mmie_t> T>
            requires(std::derived_from<T, mu_depending_t> == true)
        [[nodiscard]] T& get(const std::size_t i, const uint32_t mu) noexcept {
            auto& vec = pool[typeid(T)];
            auto* ptr = vec.at(i).get();
            T* ret = static_cast<T*>(ptr);
            ret->set_mu(mu);
            return *ret;
        }

        /**
         * \brief Gets an MMIE based on two indexes. First index defines the type, second one the
         * instance. This is useful if we iterate over MMIEs and used only for testing purposes.
         *
         * \param i defines type
         * \param j defines instance of above type saved in pool
         * \return
         */
        [[nodiscard]] mmie_t& get_by_index(const std::size_t i,
                                           const std::size_t j = 0) const noexcept {
            auto vec = std::next(pool.begin(), i);
            return *(vec->second.at(j).get());
        }

        /**
         * \brief Fill final unused bytes with paddings IEs. At the receiver, the first padding IE
         * should terminate the decoding.
         *
         * \param mac_pdu_offset
         * \param N_bytes_to_fill
         */
        void fill_with_padding_ies(uint8_t* mac_pdu_offset,
                                   const uint32_t N_bytes_to_fill) noexcept;

    protected:
        /**
         * \brief Unordered map that represents the actual pool. The container maps a MMIE type to a
         * vector of unique base pointers, each pointing to an instance of the indexed MMIE type.
         */
        std::unordered_map<std::type_index, std::vector<std::unique_ptr<mmie_t>>> pool;
};

}  // namespace dectnrp::sp4
