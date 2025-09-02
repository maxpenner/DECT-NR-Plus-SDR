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

#include "dectnrp/common/randomgen.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>

namespace dectnrp::common {

randomgen_t::randomgen_t() {
    set_repetitions(13, 17);
    set_seed(0);
}

void randomgen_t::set_repetitions(const uint32_t nof_xor128_internal_iterations_,
                                  const uint32_t nof_xor128_calls_after_seed_change_) {
    if (nof_xor128_internal_iterations_ > 0) {
        nof_xor128_internal_iterations = nof_xor128_internal_iterations_;
    } else {
        nof_xor128_internal_iterations = 1;
    }

    nof_xor128_calls_after_seed_change = nof_xor128_calls_after_seed_change_;
}

void randomgen_t::set_seed(const uint32_t seed) {
    reset();

    x = seed;

    for (uint32_t i = 0; i < nof_xor128_calls_after_seed_change; ++i) {
        xor128();
    }
}

void randomgen_t::shuffle() {
    const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();

    const uint64_t lower = now & static_cast<uint64_t>(0xffffffff);
    const uint64_t upper = now >> 32;

    const uint64_t time_dependent_seed = lower ^ upper;

    set_seed(static_cast<uint32_t>(time_dependent_seed));
}

float randomgen_t::rand() {
    xor128();

    return convert_unsigned_to_float_range_0_1<decltype(w), float>(w);
}

float randomgen_t::rand_m1p1() { return rand() * 2.0f - 1.0f; }

float randomgen_t::rand_m1p1_cpp() {
    return ((float(std::rand()) / float(RAND_MAX)) * 2.0f) - 1.0f;
}

float randomgen_t::randn() {
    float val, ret;

    do {
        val = rand();
    } while (!box_muller(ret, val));

    return ret;
}

float randomgen_t::randn(const float mu, const float std) { return mu + std * randn(); }

uint32_t randomgen_t::randi(const uint32_t imin, const uint32_t imax) {
    xor128();

    const uint32_t range = imax - imin + 1;

    return w % range + imin;
}

void randomgen_t::reset() {
    // source: https://de.wikipedia.org/wiki/Xorshift
    x = 123456789;
    y = 362436069;
    z = 521288629;
    w = 88675123;
}

void randomgen_t::xor128() {
    // source: https://de.wikipedia.org/wiki/Xorshift

    for (uint32_t i = 0; i < nof_xor128_internal_iterations; ++i) {
        const uint32_t t = x ^ (x << 11);
        x = y;
        y = z;
        z = w;

        const uint32_t a = w >> 19;
        const uint32_t b = t >> 8;
        const uint32_t c = t ^ b;

        w = w ^ a ^ c;
    }
}

bool randomgen_t::box_muller(float& out, const float in) {
    const double in_d = static_cast<double>(in);

    // must be in interval (0,1)
    if (in_d <= 0.0 || 1.0 <= in_d) {
        return false;
    }

    const double a = std::log(in_d);

    if (!std::isfinite(a) || a > 0.0) {
        return false;
    }

    out = std::sqrt(-2.0 * a) * std::cos(2.0 * M_PI * in_d);

    return true;
}

}  // namespace dectnrp::common
