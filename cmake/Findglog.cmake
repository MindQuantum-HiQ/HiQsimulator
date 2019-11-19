#.rst:
# Findglog
# -------------
#
# Find the Google glog logging library and its include directories
#
# Use this module by invoking find_package with the form::
#
#   find_package(glog
#     [REQUIRED]             # Fail with error if glog is not found
#     [QUIET]                # Find library quietly
#     )
#
# This module defines::
#
#   glog_FOUND            - True if headers and requested libraries were found
#   glog_INCLUDE_DIRS     - glog include directories
#   glog_LIBRARIES        - glog libraries to be linked
#
#
# This module reads hints about search locations from variables
# (either CMake variables or environment variables)::
#
#   glog_ROOT             - Preferred installation prefix for glog
#   glog_DIR              - Preferred installation prefix for glog
#
#
# The following :prop_tgt:`IMPORTED` targets are also defined::
#
#   glog::glog         - Imported target for the glog library
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

set(glog_SEARCH_PATHS
    ${glog_ROOT}
    $ENV{glog_ROOT}
    ${glog_DIR}
    $ENV{glog_DIR}
    ${CMAKE_PREFIX_PATH}
    $ENV{CMAKE_PREFIX_PATH}
    /usr
    /usr/local/
    /usr/local/homebrew # Mac OS X
    /opt
    /opt/local
    /opt/local/var/macports/software # Mac OS X.
    )

set(glog_LIB_PATH_SUFFIXES
    lib64
    lib
    lib/x86_64-linux-gnu
    lib32)
set(glog_INC_PATH_SUFFIXES include include/glog)

if(WIN32)
  list(APPEND glog_LIB_PATH_SUFFIXES
              Release
              MinSizeRel
              RelWithDebInfo
              Debug)
  list(APPEND glog_INC_PATH_SUFFIXES glog)
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

  get_target_property(_tmp ${target} INTERFACE_LINK_LIBRARIES)
  message(STATUS "dep libs: ${_tmp}")
endmacro(extract_inc_lib_from_target)

# ==============================================================================

find_package(gflags REQUIRED)
find_package(glog CONFIG)
if(glog_FOUND)
  message(STATUS "Found glog using the CONFIG method")
  if(TARGET glog::glog)
    extract_inc_lib_from_target(glog::glog glog)
  elseif(TARGET glog)
    extract_inc_lib_from_target(glog glog)
  else()
    message(WARNING "Finding glog using the CONFIG method failed.")
  endif()
endif()

if(glog_LIBRARY AND NOT EXISTS ${glog_LIBRARY})
  message(STATUS "glog with CONFIG method not valid")
  set(glog_FOUND FALSE)
  set(glog_LIBRARY)
  set(glog_INCLUDE_DIR)
endif()

if(NOT glog_LIBRARY)
  find_library(glog_LIBRARY
               NAMES glog libglog
               PATHS ${glog_SEARCH_PATHS}
               PATH_SUFFIXES ${glog_LIB_PATH_SUFFIXES})

  if(glog_LIBRARY)
    get_filename_component(glog_ROOT_HINT ${glog_LIBRARY} DIRECTORY)
    get_filename_component(glog_ROOT_HINT ${glog_ROOT_HINT} DIRECTORY)
  endif()

  find_path(glog_INCLUDE_DIR
            NAMES logging.h
            HINTS ${glog_ROOT_HINT}
            PATHS ${glog_SEARCH_PATHS}
            PATH_SUFFIXES ${glog_INC_PATH_SUFFIXES})

  get_filename_component(glog_INCLUDE_DIR ${glog_INCLUDE_DIR} DIRECTORY)
endif()

# ------------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  glog
  FOUND_VAR
  glog_FOUND
  REQUIRED_VARS
  glog_INCLUDE_DIR
  glog_LIBRARY
  FAIL_MESSAGE
  "Failed to find the Google logging library. You might want to specify \
either the glog_ROOT or glog_DIR CMake variables.")

# ==============================================================================

if(glog_FOUND)
  if(NOT TARGET glog::glog)
    get_filename_component(LIB_EXT "${glog_LIBRARY}" EXT)
    if(LIB_EXT STREQUAL ".a" OR LIB_EXT STREQUAL ".lib")
      set(LIB_TYPE STATIC)
    else()
      set(LIB_TYPE SHARED)
    endif()
    add_library(glog::glog ${LIB_TYPE} IMPORTED)
    set_target_properties(glog::glog
                          PROPERTIES IMPORTED_LOCATION
                                     "${glog_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${glog_INCLUDE_DIR}"
                                     INTERFACE_LINK_LIBRARIES
                                     gflags::gflags)
  else()
    # Make sure that the glog::glog target depends on gflags::gflags
    get_target_property(_libraries glog::glog INTERFACE_LINK_LIBRARIES)
    set(_new_libraries)
    foreach(_dep ${_libraries})
      if(_dep STREQUAL gflags)
        list(APPEND _new_libraries gflags::gflags)
      else()
        list(APPEND _new_libraries ${_dep})
      endif()
    endforeach()
    set_target_properties(glog::glog
                          PROPERTIES INTERFACE_LINK_LIBRARIES ${_new_libraries})
  endif()
endif()

# ------------------------------------------------------------------------------

# For debugging
if(NOT glog_FIND_QUIETLY)
  message(STATUS "Found glog and defined the glog::glog imported target:")
  message(STATUS "  - include:      ${glog_INCLUDE_DIR}")
  message(STATUS "  - library:      ${glog_LIBRARY}")
endif()

# ==============================================================================

mark_as_advanced(glog_LIBRARY glog_INCLUDE_DIR)

# ==============================================================================
