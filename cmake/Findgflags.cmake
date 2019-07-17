#.rst:
# Findgflags
# -------------
#
# Find the Google gflags logging library and its include directories
#
# Use this module by invoking find_package with the form::
#
#   find_package(gflags
#     [REQUIRED]             # Fail with error if gflags is not found
#     [QUIET]                # Find library quietly
#     )
#
# This module defines::
#
#   gflags_FOUND            - True if headers and requested libraries were found
#   gflags_INCLUDE_DIRS     - gflags include directories
#   gflags_LIBRARIES        - gflags libraries to be linked
#
#
# This module reads hints about search locations from variables
# (either CMake variables or environment variables)::
#
#   gflags_ROOT             - Preferred installation prefix for gflags
#   gflags_DIR              - Preferred installation prefix for gflags
#
#
# The following :prop_tgt:`IMPORTED` targets are also defined::
#
#   gflags::gflags         - Imported target for the gflags library
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

set(gflags_SEARCH_PATHS
    ${gflags_ROOT}
    $ENV{gflags_ROOT}
    ${gflags_DIR}
    $ENV{gflags_DIR}
    ${CMAKE_PREFIX_PATH}
    $ENV{CMAKE_PREFIX_PATH}
    /usr
    /usr/local/
    /usr/local/homebrew # Mac OS X
    /opt
    /opt/local
    /opt/local/var/macports/software # Mac OS X.
    )

set(gflags_LIB_PATH_SUFFIXES
    lib64
    lib
    lib/x86_64-linux-gnu
    lib32)
set(gflags_INC_PATH_SUFFIXES include include/gflags)

if(WIN32)
  list(APPEND gflags_LIB_PATH_SUFFIXES
              lib/Release
              lib/MinSizeRel
              lib/RelWithDebInfo
              lib/Debug)
endif()

# ==============================================================================

macro(extract_inc_lib_from_target target name)
  get_target_property(${name}_INCLUDE_DIR ${target}
                      INTERFACE_INCLUDE_DIRECTORIES)
  set(${name}_LIBRARY)
  foreach(prop
          IMPORTED_LOCATION
          IMPORTED_LOCATION_RELEASE
          IMPORTED_LOCATION_MINSIZEREL
          IMPORTED_LOCATION_RELWITHDEBINFO
          IMPORTED_LOCATION_DEBUG)
    get_target_property(${name}_LIBRARY ${target} ${prop})
    if(${name}_LIBRARY)
      break()
    endif()
  endforeach(prop)
endmacro(extract_inc_lib_from_target)

# ==============================================================================

set(GFLAGS_USE_TARGET_NAMESPACE TRUE)
find_package(gflags CONFIG)
if(gflags_FOUND)
  if(TARGET gflags::gflags)
    extract_inc_lib_from_target(gflags::gflags gflags)
  elseif(TARGET gflags)
    extract_inc_lib_from_target(gflags gflags)
  elseif(DEFINED gflags_LIBRARIES AND DEFINED gflags_INCLUDE_DIR)
    set(gflags_LIBRARY ${gflags_LIBRARIES})
  else()
    message(WARNING "Finding gflags using the CONFIG method failed.")
  endif()
endif()

if(NOT gflags_LIBRARY)
  find_library(gflags_LIBRARY
               NAMES gflags libgflags
               PATHS ${gflags_SEARCH_PATHS}
               PATH_SUFFIXES ${gflags_LIB_PATH_SUFFIXES})

  if(gflags_LIBRARY)
    get_filename_component(gflags_ROOT_HINT ${gflags_LIBRARY} DIRECTORY)
    get_filename_component(gflags_ROOT_HINT ${gflags_ROOT_HINT} DIRECTORY)
    if(WIN32)
      get_filename_component(gflags_ROOT_HINT ${gflags_ROOT_HINT} DIRECTORY)
    endif()
  endif()

  find_path(gflags_INCLUDE_DIR
            NAMES gflags.h
            HINTS ${gflags_ROOT_HINT}
            PATHS ${gflags_SEARCH_PATHS}
            PATH_SUFFIXES ${gflags_INC_PATH_SUFFIXES})
endif()

# ------------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  gflags
  FOUND_VAR
  gflags_FOUND
  REQUIRED_VARS
  gflags_INCLUDE_DIR
  gflags_LIBRARY
  FAIL_MESSAGE
  "Failed to find the gflags library. You might want to specify \
either the gflags_ROOT or gflags_DIR CMake variables.")

# ==============================================================================

if(gflags_FOUND)
  if(NOT TARGET gflags::gflags)
    get_filename_component(LIB_EXT "${gflags_LIBRARY}" EXT)
    if(LIB_EXT STREQUAL ".a" OR LIB_EXT STREQUAL ".lib")
      set(LIB_TYPE STATIC)
    else()
      set(LIB_TYPE SHARED)
    endif()
    add_library(gflags::gflags ${LIB_TYPE} IMPORTED)
    set_target_properties(gflags::gflags
                          PROPERTIES IMPORTED_LOCATION
                                     "${gflags_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${gflags_INCLUDE_DIR}")
  endif()
endif()

# ------------------------------------------------------------------------------

# For debugging
if(NOT gflags_FIND_QUIETLY)
  message(STATUS "Found gflags and defined the gflags::gflags imported target:")
  message(STATUS "  - include:      ${gflags_INCLUDE_DIR}")
  message(STATUS "  - library:      ${gflags_LIBRARY}")
endif()

# ==============================================================================

mark_as_advanced(gflags_LIBRARY gflags_INCLUDE_DIR)

# ==============================================================================
