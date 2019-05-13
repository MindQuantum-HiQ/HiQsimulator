#.rst:
# Findhwloc
# -------------
#
# Find the portable hardware locality library and its include directories
#
# Use this module by invoking find_package with the form::
#
#   find_package(hwloc
#     [REQUIRED]             # Fail with error if hwloc is not found
#     [QUIET]                # Find library quietly
#     )
#
# This module defines::
#
#   hwloc_FOUND            - True if headers and requested libraries were found
#   hwloc_INCLUDE_DIRS     - hwloc include directories
#   hwloc_LIBRARIES        - hwloc libraries to be linked
#
#
# This module reads hints about search locations from variables
# (either CMake variables or environment variables)::
#
#   hwloc_ROOT             - Preferred installation prefix for hwloc
#   hwloc_DIR              - Preferred installation prefix for hwloc
#
#
# The following :prop_tgt:`IMPORTED` targets are also defined::
#
#   hwloc::hwloc         - Imported target for the hwloc library
#

# ==============================================================================
#
# Copyright 2019 <Huawei Technologies Co., Ltd>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

set(hwloc_SEARCH_PATHS
    ${hwloc_ROOT}
    $ENV{hwloc_ROOT}
    ${hwloc_DIR}
    $ENV{hwloc_DIR}
    ${CMAKE_PREFIX_PATH}
    $ENV{CMAKE_PREFIX_PATH}
    /usr
    /usr/local/
    /usr/local/homebrew # Mac OS X
    /opt
    /opt/local
    /opt/local/var/macports/software # Mac OS X.
    )

set(LIB_PATH_SUFFIXES
    lib64
    lib
    lib/x86_64-linux-gnu
    lib32)

set(INC_PATH_SUFFIXES include)

# ==============================================================================

find_library(hwloc_LIBRARY
             NAMES hwloc libhwloc
             PATHS ${hwloc_SEARCH_PATHS}
             PATH_SUFFIXES ${LIB_PATH_SUFFIXES})

if(hwloc_LIBRARY)
  get_filename_component(hwloc_ROOT_HINT ${hwloc_LIBRARY} DIRECTORY)
  get_filename_component(hwloc_ROOT_HINT ${hwloc_ROOT_HINT} DIRECTORY)
endif()

find_path(hwloc_INCLUDE_DIR
          NAMES hwloc.h
          HINTS ${hwloc_ROOT_HINT}
          PATHS ${hwloc_SEARCH_PATHS}
          PATH_SUFFIXES ${INC_PATH_SUFFIXES})

# ==============================================================================

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  hwloc
  FOUND_VAR
  hwloc_FOUND
  REQUIRED_VARS
  hwloc_INCLUDE_DIR
  hwloc_LIBRARY
  FAIL_MESSAGE
  "Failed to find the hwloc library. You might want to specify \
either the hwloc_ROOT or hwloc_DIR CMake variables.")

# ==============================================================================

if(hwloc_FOUND)
  if(NOT TARGET hwloc::hwloc)
    get_filename_component(LIB_EXT "${hwloc_LIBRARY}" EXT)
    if(LIB_EXT STREQUAL ".a" OR LIB_EXT STREQUAL ".lib")
      set(LIB_TYPE STATIC)
    else()
      set(LIB_TYPE SHARED)
    endif()
    add_library(hwloc::hwloc ${LIB_TYPE} IMPORTED)
    set_target_properties(hwloc::hwloc
                          PROPERTIES IMPORTED_LOCATION
                                     "${hwloc_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${hwloc_INCLUDE_DIR}")
  endif()

  # ----------------------------------------------------------------------------
  # For debugging
  if(NOT hwloc_FIND_QUIETLY)
    message(STATUS "Found hwloc and defined the hwloc::hwloc imported target:")
    message(STATUS "  - include:      ${hwloc_INCLUDE_DIR}")
    message(STATUS "  - library:      ${hwloc_LIBRARY}")
  endif()
endif()

# ==============================================================================

mark_as_advanced(hwloc_LIBRARY hwloc_INCLUDE_DIR)

# ==============================================================================
