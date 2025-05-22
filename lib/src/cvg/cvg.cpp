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

#include "dectnrp/cvg/cvg.hpp"

#include "dectnrp/common/layer/layer_unit.hpp"
#include "dectnrp/cvg/io/inp_res.hpp"

namespace dectnrp::cvg {

cvg_t::cvg_t(const size_t id_, const cvg_config_t cvg_config_)
    : common::layer_unit_t("cvg", id_),
      cvg_config(cvg_config_) {}

common::adt::expected_t<handle_t, request_error_t> cvg_t::request_handle(
    [[maybe_unused]] const request_t request) {
    return handle_t();
}

inp_res_t cvg_t::write([[maybe_unused]] const handle_t& handle, [[maybe_unused]] const inp_t& inp) {
    return inp_res_t();
}

out_res_t cvg_t::read([[maybe_unused]] const handle_t& handle, [[maybe_unused]] out_t& out) {
    return out_res_t();
}

close_res_t cvg_t::close([[maybe_unused]] const handle_t& handle) {
    //

    return close_res_t();
}

std::vector<std::string> cvg_t::start_threads() {
    //

    return std::vector<std::string>();
}

std::vector<std::string> cvg_t::stop_threads() {
    //

    return std::vector<std::string>();
}

}  // namespace dectnrp::cvg
