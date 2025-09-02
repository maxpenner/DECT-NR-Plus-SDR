#
# Copyright 2013-2025 Software Radio Systems Limited (see srsRAN_Project/CMakeLists.txt)
# Copyright 2023-present Maxim Penner (changes made to srsRAN_Project/CMakeLists.txt)
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

########################################################################
# Prevent in-tree builds
########################################################################
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree build. This is bad practice.")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.20)
project(DECTNRP)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
include(version)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
   message(STATUS "Build type not specified: defaulting to Release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

# Generate CMake to include build information
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/modules/buildinfo.cmake.in
  ${CMAKE_BINARY_DIR}/buildinfo.cmake
)

include(${CMAKE_BINARY_DIR}/buildinfo.cmake)

########################################################################
# Options
########################################################################
option(ENABLE_WERROR      "Stop compilation on warnings"              ON)
option(ENABLE_ASAN        "Enable gcc/clang address sanitizer"        OFF)
option(ENABLE_TSAN        "Enable clang thread sanitizer"             OFF)
option(ENABLE_GCOV        "Enable code coverage"                      OFF)
option(ENABLE_MSAN        "Enable clang memory sanitizer"             OFF)
option(ENABLE_GPROF       "Enable gprof"                              OFF)
option(ENABLE_EXPORT      "Enable PIC and export libraries"           OFF)
option(ENABLE_TIDY        "Enable clang tidy"                         OFF)
option(AUTO_DETECT_ISA    "Autodetect supported ISA extensions"       ON)
option(BUILD_STATIC       "Attempt to statically link external deps"  OFF)
option(ENABLE_ASSERT      "Enable asserts"                            ON)
option(ENABLE_LOG         "Enable logging into file"                  ON)

if (ENABLE_WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif ()

if (ENABLE_ASAN AND ENABLE_TSAN)
  message(FATAL_ERROR "ASAN and TSAN cannot be enabled at the same time.")
endif ()

if (ENABLE_EXPORT)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif (ENABLE_EXPORT)

if (ENABLE_ASSERT)
  add_definitions(-DENABLE_ASSERT)
endif()

if (ENABLE_LOG)
  add_definitions(-DENABLE_LOG)
endif()

########################################################################
# Install Dirs
########################################################################

# nothing to do here

########################################################################
# Compiler specific setup
########################################################################
macro(ADD_CXX_COMPILER_FLAG_IF_AVAILABLE flag have)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(${flag} ${have})
  if (${have})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
  endif (${have})
endmacro(ADD_CXX_COMPILER_FLAG_IF_AVAILABLE)

# Make sure no instance of abstract class is left without a destructor
ADD_CXX_COMPILER_FLAG_IF_AVAILABLE(-Wnon-virtual-dtor HAVE_NON_VIRTUAL_DTOR)

# Make sure all overridden methods are marked as override
ADD_CXX_COMPILER_FLAG_IF_AVAILABLE(-Wsuggest-override HAVE_SUGGEST_OVERRIDE)

# Avoid shadow variables which can be caused due to C code ported into C++
ADD_CXX_COMPILER_FLAG_IF_AVAILABLE(-Wshadow HAVE_SHADOW)

# Set compiler flags for different build types.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -O0 -DDEBUG_MODE -DBUILD_TYPE_DEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ggdb -DBUILD_TYPE_RELWITHDEBINFO")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-trapping-math -fno-math-errno -DBUILD_TYPE_RELEASE")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")
#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,relro,-z,now,-z,defs")

if(ENABLE_GPROF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif(ENABLE_GPROF)

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  if (HAVE_SSE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -funroll-loops")
  endif (HAVE_SSE)
endif (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")

if (ENABLE_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  # Note: When using ASAN, we need to ensure the use of RPATH instead of RUNPATH via "-Wl,--disable-new-dtags"
  # While RPATH is default, some systems (e.g. Ubuntu 18.04 and 20.04) use RUNPATH by default, which is non-transitive.
  # Since ASAN intercepts dlopen(), by which it replaces the dynamic string token "$ORIGIN" to its own location,
  # the RF plugins won't be found when using ASAN + RUNPATH in the top-level executable.
  ADD_CXX_COMPILER_FLAG_IF_AVAILABLE("-Wl,--disable-new-dtags" HAVE_RPATH_FORCE)
endif (ENABLE_ASAN)

if (ENABLE_TSAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
  add_definitions(-DENABLE_TSAN)
endif (ENABLE_TSAN)

if (ENABLE_GCOV)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif (ENABLE_GCOV)

########################################################################
# Find dependencies
########################################################################
# Threads
find_package(Threads REQUIRED)

# FFT
find_package(FFTW3F REQUIRED)
include_directories(${FFTW3F_INCLUDE_DIRS})
link_directories(${FFTW3F_LIBRARY_DIRS})
if(BUILD_STATIC)
  set(FFT_LIBRARIES "${FFTW3F_STATIC_LIBRARIES}")
else(BUILD_STATIC)
  set(FFT_LIBRARIES "${FFTW3F_LIBRARIES}")
endif(BUILD_STATIC)
message(STATUS "FFT_LIBRARIES: " ${FFT_LIBRARIES})

# Boost
find_package(Boost 1.70.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# UHD
find_package(UHD REQUIRED)
include_directories(${UHD_INCLUDE_DIRS})
link_directories(${UHD_LIBRARY_DIRS})

# srsRAN_4G
find_package(SRSRAN REQUIRED)
include_directories(${SRSRAN_INCLUDE_DIRS})
link_directories(${SRSRAN_LIBRARY_DIRS})

# VOLK, source: CMakeLists.txt from https://github.com/gnuradio/gnuradio
find_package(Volk REQUIRED)
set(VOLK_MIN_VERSION "3.1.0")
if("${Volk_VERSION}" STREQUAL "")
  message(WARNING "Empty VOLK version string. Assuming compatibility. Good luck!")
else()
  if("${Volk_VERSION}" VERSION_LESS ${VOLK_MIN_VERSION})
    message(FATAL_ERROR "VOLK version ${Volk_VERSION} < ${VOLK_MIN_VERSION}")
  endif()
endif()

# Eigen, source: https://eigen.tuxfamily.org/dox/TopicCMakeGuide.html
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
set(EIGEN3_DIR /usr/include/eigen3)
include_directories(${EIGEN3_DIR})

########################################################################
# Instruction Set Architecture setup
########################################################################
if (AUTO_DETECT_ISA)
  if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    set(GCC_ARCH armv8-a CACHE STRING "GCC compile for specific architecture.")
    message(STATUS "Detected aarch64 processor")
    add_definitions(-DHAVE_NEON)
    set(HAVE_NEON True CACHE BOOL "NEON Instruction set is supported")
  else (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    set(GCC_ARCH native CACHE STRING "GCC compile for specific architecture.")
  endif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")

  find_package(SSE)

  # Add default architecture if enabled and available.
  ADD_CXX_COMPILER_FLAG_IF_AVAILABLE("-march=${GCC_ARCH}" HAVE_MARCH)

  if (HAVE_AVX2)
    add_definitions(-DHAVE_AVX2)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
  endif (HAVE_AVX2)
  if (HAVE_AVX)
    add_definitions(-DHAVE_AVX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
  endif (HAVE_AVX)
  if (HAVE_SSE)
    add_definitions(-DHAVE_SSE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
  endif (HAVE_SSE)
  if (HAVE_FMA)
    add_definitions(-DHAVE_FMA)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfma")
  endif (HAVE_FMA)
  if (HAVE_AVX512)
    add_definitions(-DHAVE_AVX512)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -mavx512cd -mavx512bw -mavx512dq")
  endif (HAVE_AVX512)
else (AUTO_DETECT_ISA)
  unset(HAVE_SSE CACHE)
  unset(HAVE_AVX CACHE)
  unset(HAVE_AVX2 CACHE)
  unset(HAVE_FMA CACHE)
  unset(HAVE_AVX512 CACHE)
endif (AUTO_DETECT_ISA)

########################################################################
# Compiler launcher setup
########################################################################
# Enable ccache if not already enabled
find_program(CCACHE_EXECUTABLE ccache)
mark_as_advanced(CCACHE_EXECUTABLE)
if(CCACHE_EXECUTABLE)
  foreach(LANG C CXX)
    if(NOT DEFINED CMAKE_${LANG}_COMPILER_LAUNCHER AND NOT CMAKE_${LANG}_COMPILER MATCHES ".*/ccache$")
      message(STATUS "Enabling ccache for ${LANG}")
      set(CMAKE_${LANG}_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE} CACHE STRING "")
    endif()
  endforeach()
endif()

include(CTest)
execute_process(COMMAND sed -i "s|MemoryCheckCommandOptions: |MemoryCheckCommandOptions: --verbose --trace-children=yes --time-stamp=yes --leak-check=full --show-leak-kinds=all --show-reachable=yes --exit-on-first-error=yes --error-exitcode=22 --suppressions=${CMAKE_SOURCE_DIR}/.memcheck-suppressions|" DartConfiguration.tcl
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

message(STATUS "CMAKE_CXX_FLAGS are ${CMAKE_CXX_FLAGS}")

########################################################################
# clang-tidy check
########################################################################
if(ENABLE_TIDY)
  find_program(
          CLANG_TIDY_BIN
          NAMES "clang-tidy"
          DOC "Path to clang-tidy executable"
  )
  if(NOT CLANG_TIDY_BIN)
    message(STATUS "clang-tidy not found.")
  else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_BIN}")
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
  endif()
endif(ENABLE_TIDY)

########################################################################
# Add subdirectories
########################################################################
include_directories(${PROJECT_SOURCE_DIR}/lib/include)
include_directories(${PROJECT_SOURCE_DIR}/external)
include_directories(${PROJECT_BINARY_DIR}/lib/include)
add_subdirectory(apps)
add_subdirectory(lib)
