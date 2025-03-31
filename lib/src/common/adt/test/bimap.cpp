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

#include "dectnrp/common/adt/bimap.hpp"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>

#include "dectnrp/common/prog/print.hpp"
#include "dectnrp/common/randomgen.hpp"

using namespace dectnrp;

static int test_bimap() {
    // clang-format off
    using bimap_type = boost::bimap<boost::bimaps::unordered_set_of<uint32_t>, boost::bimaps::unordered_multiset_of<uint32_t>>;
    // clang-format on

    bimap_type bimap;
    // bimap_type::left_map& lm = bimap.left;

    bimap.insert({123, 2333});
    bimap.insert({124, 333});
    bimap.insert({990, 5677});

    const auto it0 = bimap.left.find(990);
    bimap.left.replace_data(it0, 5678);

    bimap.insert({991, 5678});
    bimap.insert({992, 5678});
    bimap.insert({999, 5678});

    if (bimap.left.find(992) == bimap.left.end()) {
        return EXIT_FAILURE;
    }

    bimap.left.erase(999);

    const uint32_t value0 = bimap.left.at(123);
    dectnrp_print_inf("key={} val={}", 0, value0);

    const uint32_t value1 = bimap.left.begin()->get_right();
    dectnrp_print_inf("key={} val={}", 0, value1);

    const auto value_range = bimap.right.equal_range(5678);
    for (auto i = value_range.first; i != value_range.second; ++i) {
        dectnrp_print_inf("key={} val={}", i->get_left(), i->get_right());
    }

    dectnrp_print_inf("");
    return EXIT_SUCCESS;
}

static int test_property_unique() {
    // generate random keys and values
    dectnrp::common::randomgen_t randomgen;
    randomgen.shuffle();

    // create bimap
    common::adt::bimap_t<uint32_t, uint32_t, true> bimap;

    // ##################################################
    // insert
    std::vector<uint32_t> key_vec;
    std::vector<uint32_t> val_vec;
    for (uint32_t i = 0; i < 6; ++i) {
        const uint32_t key = randomgen.randi(1, std::numeric_limits<uint32_t>::max());
        const uint32_t val = randomgen.randi(1, std::numeric_limits<uint32_t>::max());

        bimap.insert(key, val);

        key_vec.push_back(key);
        val_vec.push_back(val);

        dectnrp_print_inf("i={}: key={} val={}", i, key, val);
    }

    // ##################################################
    // erase
    dectnrp_print_inf("Number of key={}", bimap.get_k_cnt());
    dectnrp_print_inf("Number of val={}", bimap.get_v_cnt());
    const uint32_t idx_erase = 4;
    const uint32_t key_erase = key_vec[idx_erase];
    const uint32_t val_erase = val_vec[idx_erase];
    if (!bimap.is_k_known(key_erase)) {
        return EXIT_FAILURE;
    }
    if (!bimap.is_v_known(val_erase)) {
        return EXIT_FAILURE;
    }
    bimap.erase(key_erase);
    key_vec.erase(key_vec.begin() + idx_erase);
    val_vec.erase(val_vec.begin() + idx_erase);
    dectnrp_print_inf("Number of key={}", bimap.get_k_cnt());
    dectnrp_print_inf("Number of val={}", bimap.get_v_cnt());

    // ##################################################
    // read v based on k
    uint32_t i = 0;
    for (const auto key : key_vec) {
        uint32_t val = bimap.get_v(key);
        dectnrp_print_inf("i={}: key={} val={}", i, key, val);
        ++i;
    }

    // read k based on v
    i = 0;
    for (const auto val : val_vec) {
        uint32_t key = bimap.get_k(val);
        dectnrp_print_inf("i={}: key={} val={}", i, key, val);
        ++i;
    }

    // ##################################################
    // set v

    bimap.set_v(key_vec[2], 11);

    i = 0;
    for (const auto key : key_vec) {
        uint32_t val = bimap.get_v(key);
        dectnrp_print_inf("i={}: key={} val={}", i, key, val);
        ++i;
    }

    dectnrp_print_inf("");
    return EXIT_SUCCESS;
}

static int test_property_multi() {
    // generate random keys and values
    dectnrp::common::randomgen_t randomgen;
    randomgen.shuffle();

    // create bimap
    common::adt::bimap_t<uint32_t, uint32_t, false> bimap;

    // ##################################################
    // insert
    std::vector<uint32_t> key_vec;
    std::vector<uint32_t> val_vec;
    for (uint32_t i = 0; i < 6; ++i) {
        const uint32_t key = randomgen.randi(1, std::numeric_limits<uint32_t>::max());
        const uint32_t val = randomgen.randi(1, std::numeric_limits<uint32_t>::max());

        bimap.insert(key, val);

        key_vec.push_back(key);
        val_vec.push_back(val);

        dectnrp_print_inf("i={}: key={} val={}", i, key, val);
    }

    // insert range
    const uint32_t range_val = 17171717;
    bimap.insert(999912312, range_val);
    bimap.insert(999912312 + 1, range_val);
    bimap.insert(999912312 + 2, range_val);
    bimap.insert(999912312 + 3, range_val);
    bimap.insert(999912312 + 4, range_val);
    bimap.insert(999912312 + 5, range_val);
    bimap.insert(999912312 + 6, range_val);
    bimap.insert(999912312 + 7, 99);

    if (!bimap.is_v_known(99)) {
        return EXIT_FAILURE;
    }

    // ##################################################
    // erase
    dectnrp_print_inf("Number of key={}", bimap.get_k_cnt());
    dectnrp_print_inf("Number of val={}", bimap.get_v_cnt());
    const uint32_t idx_erase = 4;
    const uint32_t key_erase = key_vec[idx_erase];
    const uint32_t val_erase = val_vec[idx_erase];
    if (!bimap.is_k_known(key_erase)) {
        return EXIT_FAILURE;
    }
    if (!bimap.is_v_known(val_erase)) {
        return EXIT_FAILURE;
    }
    bimap.erase(key_erase);
    key_vec.erase(key_vec.begin() + idx_erase);
    val_vec.erase(val_vec.begin() + idx_erase);
    dectnrp_print_inf("Number of key={}", bimap.get_k_cnt());
    dectnrp_print_inf("Number of val={}", bimap.get_v_cnt());

    // ##################################################
    // read v based on k
    uint32_t i = 0;
    for (const auto key : key_vec) {
        const uint32_t val = bimap.get_v(key);
        dectnrp_print_inf("i={}: key={} val={}", i, key, val);
        ++i;
    }

    // read k range based on v
    i = 0;
    const auto key_range_it = bimap.get_k_range(range_val);
    for (auto it = key_range_it.first; it != key_range_it.second; ++it) {
        dectnrp_print_inf("Range: i={}: key={} val={}", i, it->second, it->first);
        ++i;
    }

    // ##################################################
    // set v

    bimap.set_v(key_vec[2], 11);

    i = 0;
    for (const auto key : key_vec) {
        const uint32_t val = bimap.get_v(key);
        dectnrp_print_inf("i={}: key={} val={}", i, key, val);
        ++i;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    if (test_bimap() == EXIT_FAILURE) {
        dectnrp_print_wrn("Test failed");
        return EXIT_FAILURE;
    }

    if (test_property_unique() == EXIT_FAILURE) {
        dectnrp_print_wrn("Test failed");
        return EXIT_FAILURE;
    }

    if (test_property_multi() == EXIT_FAILURE) {
        dectnrp_print_wrn("Test failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
