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

add_library(dectnrp_mac STATIC)
target_link_libraries(dectnrp_mac dectnrp_common dectnrp_sections_part_2 dectnrp_sections_part_4)

file(GLOB DECTNRP_MAC_SOURCES "*.cpp")
target_sources(dectnrp_mac PRIVATE ${DECTNRP_MAC_SOURCES})

add_subdirectory(allocation)
add_subdirectory(pll)
add_subdirectory(ppx)