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

add_library(dectnrp_radio STATIC)

if (ENABLE_SOAPYSDR)
    target_link_libraries(dectnrp_radio dectnrp_common dectnrp_simulation srsran_phy ${UHD_LIBRARIES} SoapySDR)
else()
    target_link_libraries(dectnrp_radio dectnrp_common dectnrp_simulation srsran_phy ${UHD_LIBRARIES})
endif()

file(GLOB DECTNRP_RADIO_SOURCES "*.cpp")

if (NOT ENABLE_SOAPYSDR)
    list(REMOVE_ITEM DECTNRP_RADIO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/hw_soapy.cpp)
endif()

target_sources(dectnrp_radio PRIVATE ${DECTNRP_RADIO_SOURCES})