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

#define FMT_HEADER_ONLY
#include "dectnrp/external/fmt/color.h"
#include "dectnrp/external/fmt/core.h"
#include "dectnrp/external/fmt/format.h"

#define COMMON_PROG_PRINTS_WITH_FILEPATH_OR_FILENAME_ONLY

#ifdef COMMON_PROG_PRINTS_WITH_FILEPATH_OR_FILENAME_ONLY

#define dectnrp_print_inf(_fmt, ...)       \
    fmt::print(fmt::fg(fmt::color::green), \
               "[INF] {}:{}: {}\n",        \
               __FILE__,                   \
               __LINE__,                   \
               fmt::format(FMT_STRING(_fmt), ##__VA_ARGS__))

#define dectnrp_print_wrn(_fmt, ...)        \
    fmt::print(fmt::fg(fmt::color::orange), \
               "[WRN] {}:{}: {}\n",         \
               __FILE__,                    \
               __LINE__,                    \
               fmt::format(FMT_STRING(_fmt), ##__VA_ARGS__))

#else

// __FILE_NAME__ is supported by GCC and Clang

// https://github.com/microsoft/vscode-cpptools/issues/11164

#define dectnrp_print_inf(_fmt, ...)       \
    fmt::print(fmt::fg(fmt::color::green), \
               "[INF] {}:{}: {}\n",        \
               __FILE_NAME__,              \
               __LINE__,                   \
               fmt::format(FMT_STRING(_fmt), ##__VA_ARGS__))

#define dectnrp_print_wrn(_fmt, ...)        \
    fmt::print(fmt::fg(fmt::color::orange), \
               "[WRN] {}:{}: {}\n",         \
               __FILE_NAME__,               \
               __LINE__,                    \
               fmt::format(FMT_STRING(_fmt), ##__VA_ARGS__))

#endif
