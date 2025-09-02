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

#include "dectnrp/common/json/json_export.hpp"

#include <fstream>
#include <sstream>

#include "dectnrp/common/prog/assert.hpp"

namespace dectnrp::common {

void json_export_t::append(const nlohmann::ordered_json&& json_to_append) {
    // save the index of the JSON we have to write to the disk
    int32_t json_arr_write_disk = -1;

    // if we write, we define the filename under lock
    std::string filename;

    {
        // std::unique_lock not possible since this could be a spinlock
        lockv_json.lock();

        nlohmann::ordered_json& json = json_arr.at(json_arr_write);

        // add new entry to JSON
        json[prefix_entry + std::to_string(postfix_entry)].update(json_to_append, false);

        ++postfix_entry;

        // check if vector is full
        if ((postfix_entry % json_length) == 0) {
            /* It can happen that another thread has not yet finished writing the other JSON to the
             * disk. We only try to get the lock as we only allow one thread to write to the disk.
             */
            if (lockv_disk.try_lock()) {
                // we write the active json (which is now full) to the disk with ...
                json_arr_write_disk = json_arr_write;

                // ... this filename
                filename = prefix_file +
                           get_number_with_leading_zeros(postfix_file++, N_postfix_file_characters);

                // swap
                json_arr_write = (json_arr_write == 0) ? 1 : 0;
            } else {
                // another thread still writing, so clear current JSON and by that discard JSON
                json_arr.at(json_arr_write_disk).clear();

                ++stats.lockv_disk_fail;
            }
        }

        lockv_json.unlock();
    }

    // if json_arr_write_disk was overwritten, we are holding the lock on lockv_disk
    if (json_arr_write_disk >= 0) {
        write_to_disk(json_arr.at(json_arr_write_disk), filename);

        // JSON written, so clear it
        json_arr.at(json_arr_write_disk).clear();

        lockv_disk.unlock();
    }
}

void json_export_t::write_to_disk(const nlohmann::ordered_json& json, const std::string& filename) {
    dectnrp_assert(!json.empty(), "JSON empty");
    dectnrp_assert(!filename.empty(), "filename empty");

    std::ofstream out_file(filename);
    out_file << std::setw(4) << json << std::endl;
    out_file.close();
}

std::string json_export_t::get_number_with_leading_zeros(const uint32_t number,
                                                         const uint32_t N_characters) {
    std::stringstream ret;
    ret << std::setw(N_characters) << std::setfill('0') << number;
    return ret.str();
}

std::vector<std::string> json_export_t::report_start() const {
    std::vector<std::string> lines;

    std::string str("JSON Export");
    str.append(" #JSONs " + std::to_string(json_length));

    lines.push_back(str);

    return lines;
}

std::vector<std::string> json_export_t::report_stop() const {
    std::vector<std::string> lines;

    std::string str("JSON Export");
    str.append(" lockv_disk_fail " + std::to_string(stats.lockv_disk_fail));

    lines.push_back(str);

    return lines;
}

}  // namespace dectnrp::common
