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

#include "dectnrp/common/prog/defined.hpp"

#define FMT_HEADER_ONLY
#define FMTLOG_HEADER_ONLY
#include "header_only/fmt/core.h"
#include "header_only/fmt/format.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include "header_only/fmtlog/fmtlog.h"
#pragma GCC diagnostic pop

namespace dectnrp::log {

void setup(const char* logfilename);
void save();

}  // namespace dectnrp::log

// clang-format off
 #ifdef ENABLE_LOG
 #define dectnrp_log_setup(logfilename) dectnrp::log::setup(logfilename)
 #define dectnrp_log(logLevel, fmtstr, ...) FMTLOG(logLevel, "{}", fmt::format(FMT_STRING(fmtstr), ##__VA_ARGS__));
 #define dectnrp_log_inf(fmtstr, ...) FMTLOG(fmtlog::LogLevel::INF, "{}", fmt::format(FMT_STRING(fmtstr), ##__VA_ARGS__));
 #define dectnrp_log_wrn(fmtstr, ...) FMTLOG(fmtlog::LogLevel::WRN, "{}", fmt::format(FMT_STRING(fmtstr), ##__VA_ARGS__));
 #define dectnrp_log_save() dectnrp::log::save()
 #else
 #define dectnrp_log_setup(logfilename)
 #define dectnrp_log(logLevel, fmtstr, ...)
 #define dectnrp_log_inf(fmtstr, ...)
 #define dectnrp_log_wrn(fmtstr, ...)
 #define dectnrp_log_save()
 #endif
// clang-format on
