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

// SOURCE: https://github.com/srsran/srsRAN_Project/blob/main/include/srsran/support/srsran_assert.h

#include <cstdio>
#include <string>

#include "dectnrp/common/prog/defined.hpp"

#define FMT_HEADER_ONLY
#include "dectnrp/external/fmt/color.h"
#include "dectnrp/external/fmt/core.h"
#include "dectnrp/external/fmt/format.h"

namespace dectnrp::assert {

[[gnu::noinline, noreturn]] inline void print_and_abort(const char* filename,
                                                        int line,
                                                        const char* funcname,
                                                        const char* condstr = nullptr,
                                                        const std::string& msg = "") noexcept {
    fmt::print(fmt::fg(fmt::color::red),
               "[ASSERT] {}:{}:{}: {} | {}\n",
               filename,
               line,
               funcname,
               condstr,
               msg);

    std::abort();
}

}  // namespace dectnrp::assert

/// Helper macro to log assertion message and terminate program.
#define DECTNRP_ASSERT_FAILURE__(condmessage, fmtstr, ...) \
    dectnrp::assert::print_and_abort(__FILE__,             \
                                     __LINE__,             \
                                     __PRETTY_FUNCTION__,  \
                                     condmessage,          \
                                     fmt::format(FMT_STRING(fmtstr), ##__VA_ARGS__))

// ##################################################
// assert failure without checking condition

#define dectnrp_assert_failure(fmtstr, ...)            \
    (void)((not DECTNRP_IS_DEFINED(ASSERT_ENABLED)) || \
           (DECTNRP_ASSERT_FAILURE__("no error condition", fmtstr, ##__VA_ARGS__), 0))

// ##################################################
// assert with checking condition

#define DECTNRP_ALWAYS_ASSERT__(condition, fmtstr, ...) \
    (void)((condition) || (DECTNRP_ASSERT_FAILURE__((#condition), fmtstr, ##__VA_ARGS__), 0))

#define DECTNRP_ALWAYS_ASSERT_IFDEF__(enable_check, condition, fmtstr, ...) \
    (void)((not DECTNRP_IS_DEFINED(enable_check)) ||                        \
           (DECTNRP_ALWAYS_ASSERT__(condition, fmtstr, ##__VA_ARGS__), 0))

#define dectnrp_assert(condition, fmtstr, ...) \
    DECTNRP_ALWAYS_ASSERT_IFDEF__(ASSERT_ENABLED, condition, fmtstr, ##__VA_ARGS__)
