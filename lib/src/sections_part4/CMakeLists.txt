#
# Copyright 2023-2025 Maxim Penner
#
# This file is part of DECTNRP.
#
# DECTNRP is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# DECTNRP is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

add_library(dectnrp_sections_part_4 STATIC)
target_link_libraries(dectnrp_sections_part_4 dectnrp_common dectnrp_sections_part_2)

file(GLOB DECTNRP_SECTIONS_PART_4_SOURCES "*.cpp")
target_sources(dectnrp_sections_part_4 PRIVATE ${DECTNRP_SECTIONS_PART_4_SOURCES})

add_subdirectory(mac_architecture)
add_subdirectory(mac_messages_and_ie)
add_subdirectory(mac_pdu)
add_subdirectory(physical_header_field)