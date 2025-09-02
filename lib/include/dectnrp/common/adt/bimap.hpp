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

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <cmath>
#include <concepts>
#include <optional>
#include <type_traits>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common::adt {

template <std::unsigned_integral K, typename V, bool unique>
    requires(std::is_integral_v<V> || std::is_enum_v<V> || std::is_trivially_copyable_v<V>)
class bimap_t {
    public:
        void insert(const K k, const V v) noexcept {
            dectnrp_assert(!is_k_known(k), "k already known");
            if constexpr (unique) {
                dectnrp_assert(!is_v_known(v), "v already known");
            }
            const auto p = bimap.insert({k, v});
            dectnrp_assert(p.second == true, "insert failed");
        }

        void erase(const K k) noexcept {
            dectnrp_assert(is_k_known(k), "k unknown");
            bimap.left.erase(bimap.left.find(k));
        }

        // ##################################################
        // value getter and setter

        bool is_v_known(const V& v) const noexcept {
            return bimap.right.find(v) != bimap.right.end();
        }

        void set_v(const K k, const V& v) noexcept {
            dectnrp_assert(is_k_known(k), "k unknown");
            if constexpr (unique) {
                dectnrp_assert(!is_v_known(v), "v already known");
            }
            bool b = bimap.left.replace_data(bimap.left.find(k), v);
            dectnrp_assert(b == true, "replace failed");
        }

        V get_v(const K k) const noexcept {
            dectnrp_assert(is_k_known(k), "k unknown");
            return bimap.left.at(k);
        }

        std::optional<V> get_v_as_opt(const K k) const noexcept {
            return is_k_known(k) ? bimap.left.at(k) : std::nullopt;
        }

        uint32_t get_v_cnt() const noexcept { return bimap.right.size(); };

        // ##################################################
        // key getter, key has no setter as we consider the key to be immutable

        bool is_k_known(const K k) const noexcept { return bimap.left.find(k) != bimap.left.end(); }

        K get_k(const V& v) const noexcept
            requires(unique)
        {
            dectnrp_assert(is_v_known(v), "v unknown");
            return bimap.right.at(v);
        }

        std::optional<K> get_k_as_opt(const V& v) const noexcept
            requires(unique)
        {
            return is_v_known(v) ? bimap.right.at(v) : std::nullopt;
        }

        auto get_k_range(const V& v) const noexcept
            requires(!unique)
        {
            dectnrp_assert(is_v_known(v), "v unknown");
            return bimap.right.equal_range(v);
        }

        uint32_t get_k_cnt() const noexcept { return bimap.left.size(); };

        void reserve(const std::size_t N) {
            bimap.rehash(std::ceil(static_cast<float>(N) / bimap.max_load_factor()));
        };

    private:
        // clang-format off
        using A = boost::bimap<boost::bimaps::unordered_set_of<K>, boost::bimaps::unordered_set_of<V>>;
        using B = boost::bimap<boost::bimaps::unordered_set_of<K>, boost::bimaps::unordered_multiset_of<V>>;
        // clang-format on

        std::conditional_t<unique, A, B> bimap;
};

}  // namespace dectnrp::common::adt
