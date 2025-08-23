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

#include <concepts>
#include <cstdint>
#include <limits>

namespace dectnrp::common {

class randomgen_t {
    public:
        /**
         * \brief Internally uses an XOR-based algorithm simple enough to be implemented in
         * different languages.
         *
         * source: https://de.wikipedia.org/wiki/Xorshift
         */
        randomgen_t();

        /// set number of repetitions in case Matlab uses less
        void set_repetitions(const uint32_t nof_xor128_internal_iterations_,
                             const uint32_t nof_xor128_calls_after_seed_change_);

        /// set seed for reproducible numbers (constructs with 0)
        void set_seed(const uint32_t seed);

        /// put into time dependent state
        void shuffle();

        /// uniformly distributed numbers in the range [0.0, 1.0], same algorithm is implemented in
        /// matlab
        float rand();

        /// uniformly distributed numbers in the range [-1.0, 1.0]
        float rand_m1p1();

        /// internally using std::rand()
        static float rand_m1p1_cpp();

        /// normal distribution with box-muller
        float randn();
        float randn(const float mu, const float std);

        /// uniformly distributed positive integers in range [imin, imax]
        uint32_t randi(const uint32_t imin, const uint32_t imax);

    private:
        /// c, y, z and w are set to default values
        void reset();

        /// internal state
        uint32_t x, y, z, w;

        /// for better randomness
        uint32_t nof_xor128_internal_iterations;
        uint32_t nof_xor128_calls_after_seed_change;

        /// advance internal state
        void xor128();

        template <std::unsigned_integral U, std::floating_point F>
        static F convert_unsigned_to_float_range_0_1(const U in) {
            return static_cast<F>(in) / static_cast<F>(std::numeric_limits<U>::max());
        }

        static bool box_muller(float& out, const float in);
};

}  // namespace dectnrp::common
