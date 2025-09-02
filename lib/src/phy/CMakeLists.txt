#
# Copyright 2023-present Maxim Penner
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

add_library(dectnrp_phy STATIC)
target_link_libraries(dectnrp_phy dectnrp_common dectnrp_radio dectnrp_sections_part_3 ${FFT_LIBRARIES} ${VOLK_LIBRARIES} Eigen3::Eigen)

file(GLOB DECTNRP_PHY_SOURCES "*.cpp")
target_sources(dectnrp_phy PRIVATE ${DECTNRP_PHY_SOURCES})

add_subdirectory(agc)
add_subdirectory(dft)
add_subdirectory(fec)
add_subdirectory(filter)
add_subdirectory(harq)
add_subdirectory(indicators)
add_subdirectory(mix)
add_subdirectory(pool)
add_subdirectory(resample)
add_subdirectory(rx)
add_subdirectory(tx)