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

#include <Eigen/Dense>
#include <cstdint>
#include <vector>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::phy {

template <typename T>
class wiener_t {
    public:
        explicit wiener_t(const uint32_t N_);
        ~wiener_t() = default;

        wiener_t() = delete;
        wiener_t(const wiener_t&) = delete;
        wiener_t& operator=(const wiener_t&) = delete;
        wiener_t(wiener_t&&) = delete;
        wiener_t& operator=(wiener_t&&) = delete;

        /**
         * \brief The Wiener-Hopf equation is Rpp * w = rdp of size (NxN)*(Nx1)=(Nx1).
         *
         * Rpp: correlation matrix pilot to pilot
         * w:   optimal weights for interpolation
         * rdp: correlation vector data to pilot
         *
         * \param row_idx
         * \param col_idx
         * \param val
         */
        void set_entry_Rpp(const uint32_t row_idx, const uint32_t col_idx, const T val);
        void set_entry_rdp(const uint32_t row_idx, const T val);

        /**
         * \brief Our goal is to determine w = Rppinv * rdp. Retrieving the vector w is split into
         * calculating the inverse of Rpp and the matrix-vector multiplication with rdp. This has
         * the benefit of the inverse being reusable for multiple values of rdp.
         */
        void set_Rppinv();
        std::vector<T> get_Rppinv_x_rdp(const bool normalize);

        const uint32_t N;

    private:
        /// https://eigen.tuxfamily.org/dox/group__TutorialLinearAlgebra.html
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> Rpp;
        Eigen::Vector<T, Eigen::Dynamic> rdp;

        /** \brief For our interpolation filter w, the Wiener-Hopf equation is overdetermined. For
         * instance, if the channel is flat in both time- and frequency domain (reasonable
         * assumption if the signal bandwidth is smaller than the coherence bandwidth and the
         * channel remains static over time), and if we assume real correlation between pilots, Rpp
         * is a matrix of ones (unit matrix) and rdp is a vector of ones. Such an equation has no
         * unique solution.
         *
         * In this case, it's best to use an algorithm that minimizes the value of Rpp*x-b (least
         * squares) AND the L2-norm of rdp. For our flat channel, this would result in w having all
         * equal values, and thus, optimally smoothing the noise across multiple pilots.
         *
         * In Matlab, the function lsqminnorm() can be used for this.
         * https://de.mathworks.com/help/matlab/ref/lsqminnorm.html
         *
         * When using Eigen, we can use completeOrthogonalDecomposition() to get the same result.
         */
        Eigen::CompleteOrthogonalDecomposition<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>
            Rppinv;
};

template <typename T>
wiener_t<T>::wiener_t(const uint32_t N_)
    : N(N_) {
    Rpp.resize(N, N);
    rdp.resize(N);
}

template <typename T>
void wiener_t<T>::set_entry_Rpp(const uint32_t row_idx, const uint32_t col_idx, const T val) {
    dectnrp_assert(row_idx < N, "Row index out of bound.");
    dectnrp_assert(col_idx < N, "Column index out of bound.");
    Rpp(row_idx, col_idx) = val;
}

template <typename T>
void wiener_t<T>::set_entry_rdp(const uint32_t row_idx, const T val) {
    dectnrp_assert(row_idx < N, "Row index out of bound.");
    rdp(row_idx) = val;
}

template <typename T>
void wiener_t<T>::set_Rppinv() {
    Rppinv = Rpp.completeOrthogonalDecomposition();
}

template <typename T>
std::vector<T> wiener_t<T>::get_Rppinv_x_rdp(const bool normalize) {
    // https://eigen.tuxfamily.org/dox/group__TutorialLinearAlgebra.html
    Eigen::Vector<T, Eigen::Dynamic> w = Rppinv.solve(rdp);

    // prints for debugging (add #include <iostream>)
    // std::cout << "NxN Matrix Rpp:\n" << Rpp << std::endl;
    // std::cout << "Nx1 Vector rdp:\n" << rdp << std::endl;
    // std::cout << "Nx1 Vector w:  \n" << w << std::endl;

    // normalization factor
    const T norm = normalize ? w.sum() : T(1);

    dectnrp_assert(norm != T(0), "Normalization factor is zero.");

    // convert to vector and normalize
    std::vector<T> ret(N);
    for (uint32_t row_idx = 0; row_idx < N; ++row_idx) {
        ret[row_idx] = w(row_idx) / norm;
    }

    return ret;
}

}  // namespace dectnrp::phy
