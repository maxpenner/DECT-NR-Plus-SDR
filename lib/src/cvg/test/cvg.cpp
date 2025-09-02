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

#include "dectnrp/cvg/cvg.hpp"

#include <cstdlib>

#include "dectnrp/cvg/io/inp.hpp"
#include "dectnrp/cvg/io/out.hpp"

using namespace dectnrp;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    const cvg::cvg_config_t cvg_config;

    [[maybe_unused]] cvg::cvg_t cvg(123, cvg_config);

    const cvg::request_t request;

    auto handle_res = cvg.request_handle(request);

    if (!handle_res.has_value()) {
        return EXIT_FAILURE;
    }

    cvg::inp_t inp;
    [[maybe_unused]] const auto inp_res = cvg.write(handle_res.value(), inp);

    cvg::out_t out;
    [[maybe_unused]] const auto out_res = cvg.read(handle_res.value(), out);

    [[maybe_unused]] const auto close_res = cvg.close(handle_res.value());

    return EXIT_SUCCESS;
}
