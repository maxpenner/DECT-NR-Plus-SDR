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

#include <array>
#include <cstdint>
#include <string>

#include "dectnrp/common/adt/cast.hpp"
#include "dectnrp/common/complex.hpp"
#include "dectnrp/common/reporting.hpp"
#include "header_only/nlohmann/json.hpp"

#define COMMON_JSON_JSON_EXPORT_MUTEX_OR_SPINLOCK
#ifdef COMMON_JSON_JSON_EXPORT_MUTEX_OR_SPINLOCK
#include <mutex>
#else
#include "dectnrp/common/thread/spinlock.hpp"
#endif

namespace dectnrp::common {

class json_export_t final : public common::reporting_t {
    public:
        json_export_t(const uint32_t json_length_,
                      const std::string prefix_file_,
                      const std::string prefix_entry_)
            : json_length(json_length_),
              prefix_file(prefix_file_),
              prefix_entry(prefix_entry_) {}

        json_export_t() = delete;
        json_export_t(const json_export_t&) = delete;
        json_export_t& operator=(const json_export_t&) = delete;
        json_export_t(json_export_t&&) = delete;
        json_export_t& operator=(json_export_t&&) = delete;

        static constexpr uint32_t N_postfix_file_characters{10};

        /**
         * \brief Thread-safe, but can block for a short period of time once final JSON entry is
         * appended. The blocking duration depends on the JSON size and the number of appended
         * JSONs. When exporting JSONs, there should always be at least two threads such that one
         * thread is always able to keep processing jobs.
         *
         * \param json appended as JSON entry
         */
        void append(const nlohmann::ordered_json&& json);

        static void write_to_disk(const nlohmann::ordered_json& json, const std::string& filename);

        static std::string get_number_with_leading_zeros(const uint32_t number,
                                                         const uint32_t N_characters);

        // ##################################################
        // conversion functions

        /**
         * \brief nlohmann saves float with full precision, to reduce the JSON size floats are
         * scaled and converted to integer.
         *
         * \param src container with values for conversion
         * \param N number of elements to convert
         * \tparam scaled_int32
         * \tparam stride
         * \return
         */
        template <bool scaled_int32 = false, size_t stride = 1>
        static nlohmann::ordered_json convert_32fc_re_im(const cf_t* src, const size_t N) {
            nlohmann::ordered_json json;

            for (size_t i = 0; i < N; i += stride) {
                const float re = __real__(src[i]);
                const float im = __imag__(src[i]);

                if constexpr (scaled_int32) {
                    json["re"].push_back(common::adt::cast::float_to_int(re));
                    json["im"].push_back(common::adt::cast::float_to_int(im));
                } else {
                    json["re"].push_back((re));
                    json["im"].push_back((im));
                }
            }

            return json;
        }

        /**
         * \brief nlohmann saves float with full precision, to reduce the JSON size floats are
         * scaled and converted to integer.
         *
         * \param src container with values for conversion
         * \param N number of elements to convert
         * \tparam scaled_int32
         * \tparam stride
         * \return
         */
        template <bool scaled_int32 = false, size_t stride = 1, typename T>
        static auto convert_to_vec(const T& src, const size_t N) {
            using Ttype = typename std::decay<decltype(*src.begin())>::type;

            std::conditional_t<scaled_int32, std::vector<int>, std::vector<Ttype>> ret;

            for (size_t i = 0; i < std::min(src.size(), N); i += stride) {
                if constexpr (scaled_int32) {
                    ret.push_back(common::adt::cast::float_to_int(src[i]));
                } else {
                    ret.push_back(src[i]);
                }
            }

            return ret;
        }

        std::vector<std::string> report_start() const override final;
        std::vector<std::string> report_stop() const override final;

    private:
        /// double-buffer
        std::array<nlohmann::ordered_json, 2> json_arr;
        uint32_t json_arr_write{0};

#ifdef COMMON_JSON_JSON_EXPORT_MUTEX_OR_SPINLOCK
        std::mutex lockv_json, lockv_disk;
#else
        common::spinlock_t lockv_json, lockv_disk;
#endif

        /// write JSON to disk after a certain number of appended entries
        const uint32_t json_length;

        /// every file and every JSON entry has the same name ...
        const std::string prefix_file;
        const std::string prefix_entry;

        /// ... except for the trailing number
        uint64_t postfix_file{0};
        uint64_t postfix_entry{0};

        struct stats_t {
                int64_t lockv_disk_fail{0};
        } stats;
};

}  // namespace dectnrp::common
