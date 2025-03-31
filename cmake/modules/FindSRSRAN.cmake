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

FIND_PACKAGE(PkgConfig REQUIRED)

IF(NOT SRSRAN_FOUND)

FIND_PATH(
    SRSRAN_INCLUDE_DIRS
    NAMES srsran/srsran.h
    PATHS /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SRSRAN_LIBRARIES
    NAMES srsran_phy
    PATHS /usr/local/lib
          /usr/lib
          /usr/lib/x86_64-linux-gnu
          /usr/local/lib64
          /usr/local/lib32
)

message(STATUS "SRSRAN INCLUDE DIRS " ${SRSRAN_INCLUDE_DIRS})
message(STATUS "SRSRAN LIBRARIES " ${SRSRAN_LIBRARIES})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SRSRAN DEFAULT_MSG SRSRAN_LIBRARIES SRSRAN_INCLUDE_DIRS)
MARK_AS_ADVANCED(SRSRAN_LIBRARIES SRSRAN_INCLUDE_DIRS)

include(CheckCXXSourceCompiles)

IF(SRSRAN_FOUND)
  # UHD library directory
  get_filename_component(SRSRAN_LIBRARY_DIR ${SRSRAN_LIBRARIES} DIRECTORY)

  # Save current required variables
  set(_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
  set(_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  #set(_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})

  # Set required variables
  set(CMAKE_REQUIRED_INCLUDES ${SRSRAN_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS} -L${SRSRAN_LIBRARY_DIR}")
  #set(CMAKE_REQUIRED_LIBRARIES uhd boost_program_options boost_system)

  # Recover required variables
  set(CMAKE_REQUIRED_INCLUDES ${_CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_FLAGS ${_CMAKE_REQUIRED_FLAGS})
  #set(CMAKE_REQUIRED_LIBRARIES ${_CMAKE_REQUIRED_LIBRARIES})
ENDIF(SRSRAN_FOUND)

ENDIF(NOT SRSRAN_FOUND)