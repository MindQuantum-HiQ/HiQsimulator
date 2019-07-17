#.rst:
# Findxsimd
# -------------
#
# Find the XSIMD library
#
# Use this module by invoking find_package with the form::
#
#   find_package(xsimd
#     [REQUIRED]             # Fail with error if xsimd is not found
#     [QUIET]                # Find library quietly
#     )
#
# This module defines::
#
#   xsimd_FOUND            - True if headers and requested libraries were found
#   xsimd_INCLUDE_DIRS     - xsimd include directories
#   xsimd_LIBRARIES        - xsimd libraries to be linked
#
#
# This module reads hints about search locations from variables
# (either CMake variables or environment variables)::
#
#   xsimd_ROOT             - Preferred installation prefix for xsimd
#   xsimd_DIR              - Preferred installation prefix for xsimd
#
#
# The following :prop_tgt:`IMPORTED` targets are also defined::
#
#   xsimd::xsimd         - Imported target for the xsimd library
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

set(xsimd_SEARCH_PATHS
    ${xsimd_ROOT}
    $ENV{xsimd_ROOT}
    ${xsimd_DIR}
    $ENV{xsimd_DIR}
    ${CMAKE_PREFIX_PATH}
    $ENV{CMAKE_PREFIX_PATH}
    /usr
    /usr/local/
    /usr/local/homebrew # Mac OS X
    /opt
    /opt/local
    /opt/local/var/macports/software # Mac OS X.
    )

set(xsimd_INC_PATH_SUFFIXES include include/xsimd)

# ==============================================================================

# Always prefer the submodule directory if it exists
if(EXISTS ${PROJECT_SOURCE_DIR}/xsimd)
  find_path(xsimd_INCLUDE_DIR
            NAMES xsimd.hpp
            PATHS ${PROJECT_SOURCE_DIR}/xsimd
            PATH_SUFFIXES ${xsimd_INC_PATH_SUFFIXES}
            NO_DEFAULT_PATH)
  get_filename_component(xsimd_INCLUDE_DIR ${xsimd_INCLUDE_DIR} DIRECTORY)
endif()

if(NOT xsimd_INCLUDE_DIR)
  find_package(xsimd CONFIG)
  if(xsimd_FOUND)
    set(xsimd_INCLUDE_DIR)
    foreach(target xsimd xsimd::xsimd)
      if(TARGET ${target})
        get_target_property(xsimd_INCLUDE_DIR ${target}
                            INTERFACE_INCLUDE_DIRECTORIES)
      endif()
      if(xsimd_INCLUDE_DIR)
        break()
      endif()
    endforeach(target)
  else()
    find_path(xsimd_INCLUDE_DIR
              NAMES xsimd.hpp
              PATHS ${xsimd_SEARCH_PATHS}
              PATH_SUFFIXES ${xsimd_INC_PATH_SUFFIXES})

    if(xsimd_INCLUDE_DIR)
      get_filename_component(xsimd_INCLUDE_DIR ${xsimd_INCLUDE_DIR} DIRECTORY)
    endif()
  endif()
endif()

# ------------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  xsimd
  FOUND_VAR
  xsimd_FOUND
  REQUIRED_VARS
  xsimd_INCLUDE_DIR
  FAIL_MESSAGE
  "Failed to find the XSIMD library. You might want to specify \
either the xsimd_ROOT or xsimd_DIR CMake variables.")

# ==============================================================================

if(xsimd_FOUND)
  if(NOT TARGET xsimd::xsimd)
    add_library(xsimd::xsimd IMPORTED INTERFACE)
    set_target_properties(xsimd::xsimd
                          PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                     "${xsimd_INCLUDE_DIR}")
  endif()
endif()

# ------------------------------------------------------------------------------

# For debugging
if(NOT xsimd_FIND_QUIETLY AND xsimd_FOUND)
  message(STATUS "Found xsimd and defined the xsimd::xsimd imported target:")
  message(STATUS "  - include:      ${xsimd_INCLUDE_DIR}")
endif()

# ==============================================================================

mark_as_advanced(xsimd_LIBRARY xsimd_INCLUDE_DIR)

# ==============================================================================
