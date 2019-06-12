# Taken from https://github.com/Kitware/CMake for CMake 3.12
#
# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindNOpenMP
# ----------
#
# Finds OpenMP support
#
# This module can be used to detect OpenMP support in a compiler.  If
# the compiler supports OpenMP, the flags required to compile with
# OpenMP support are returned in variables for the different languages.
# The variables may be empty if the compiler does not need a special
# flag to support OpenMP.
#
# Variables
# ^^^^^^^^^
#
# The module exposes the components ``C``, ``CXX``, and ``Fortran``.
# Each of these controls the various languages to search OpenMP support for.
#
# Depending on the enabled components the following variables will be set:
#
# ``OpenMP_FOUND``
#   Variable indicating that OpenMP flags for all requested languages have been found.
#   If no components are specified, this is true if OpenMP settings for all enabled languages
#   were detected.
# ``OpenMP_VERSION``
#   Minimal version of the OpenMP standard detected among the requested languages,
#   or all enabled languages if no components were specified.
#
# This module will set the following variables per language in your
# project, where ``<lang>`` is one of C, CXX, or Fortran:
#
# ``OpenMP_<lang>_FOUND``
#   Variable indicating if OpenMP support for ``<lang>`` was detected.
# ``OpenMP_<lang>_FLAGS``
#   OpenMP compiler flags for ``<lang>``, separated by spaces.
#
# For linking with OpenMP code written in ``<lang>``, the following
# variables are provided:
#
# ``OpenMP_<lang>_LIB_NAMES``
#   :ref:`;-list <CMake Language Lists>` of libraries for OpenMP programs for ``<lang>``.
# ``OpenMP_<libname>_LIBRARY``
#   Location of the individual libraries needed for OpenMP support in ``<lang>``.
# ``OpenMP_<lang>_LIBRARIES``
#   A list of libraries needed to link with OpenMP code written in ``<lang>``.
#
# Additionally, the module provides :prop_tgt:`IMPORTED` targets:
#
# ``OpenMP::OpenMP_<lang>``
#   Target for using OpenMP from ``<lang>``.
#
# Specifically for Fortran, the module sets the following variables:
#
# ``OpenMP_Fortran_HAVE_OMPLIB_HEADER``
#   Boolean indicating if OpenMP is accessible through ``omp_lib.h``.
# ``OpenMP_Fortran_HAVE_OMPLIB_MODULE``
#   Boolean indicating if OpenMP is accessible through the ``omp_lib`` Fortran module.
#
# The module will also try to provide the OpenMP version variables:
#
# ``OpenMP_<lang>_SPEC_DATE``
#   Date of the OpenMP specification implemented by the ``<lang>`` compiler.
# ``OpenMP_<lang>_VERSION_MAJOR``
#   Major version of OpenMP implemented by the ``<lang>`` compiler.
# ``OpenMP_<lang>_VERSION_MINOR``
#   Minor version of OpenMP implemented by the ``<lang>`` compiler.
# ``OpenMP_<lang>_VERSION``
#   OpenMP version implemented by the ``<lang>`` compiler.
#
# The specification date is formatted as given in the OpenMP standard:
# ``yyyymm`` where ``yyyy`` and ``mm`` represents the year and month of
# the OpenMP specification implemented by the ``<lang>`` compiler.

function(_NOPENMP_FLAG_CANDIDATES LANG)
  if(NOT NOpenMP_${LANG}_FLAG)
    unset(NOpenMP_FLAG_CANDIDATES)
    set(OMP_FLAG_GNU "-fopenmp")
    set(OMP_FLAG_Clang "-fopenmp=libomp" "-fopenmp=libiomp5" "-fopenmp")
    set(OMP_FLAG_AppleClang "-Xclang -fopenmp")
    set(OMP_FLAG_HP "+Oopenmp")
    if(WIN32)
      set(OMP_FLAG_Intel "-Qopenmp")
    elseif(CMAKE_${LANG}_COMPILER_ID STREQUAL "Intel"
           AND "${CMAKE_${LANG}_COMPILER_VERSION}" VERSION_LESS
               "15.0.0.20140528")
      set(OMP_FLAG_Intel "-openmp")
    else()
      set(OMP_FLAG_Intel "-qopenmp")
    endif()
    set(OMP_FLAG_MIPSpro "-mp")
    set(OMP_FLAG_MSVC "-openmp")
    set(OMP_FLAG_PathScale "-openmp")
    set(OMP_FLAG_NAG "-openmp")
    set(OMP_FLAG_Absoft "-openmp")
    set(OMP_FLAG_PGI "-mp")
    set(OMP_FLAG_Flang "-fopenmp")
    set(OMP_FLAG_SunPro "-xopenmp")
    set(OMP_FLAG_XL "-qsmp=omp")
    # Cray compiler activate OpenMP with -h omp, which is enabled by default.
    set(OMP_FLAG_Cray " " "-h omp")

    # If we know the correct flags, use those
    if(DEFINED OMP_FLAG_${CMAKE_${LANG}_COMPILER_ID})
      set(NOpenMP_FLAG_CANDIDATES "${OMP_FLAG_${CMAKE_${LANG}_COMPILER_ID}}")
      # Fall back to reasonable default tries otherwise
    else()
      set(NOpenMP_FLAG_CANDIDATES "-openmp" "-fopenmp" "-mp" " ")
    endif()
    set(NOpenMP_${LANG}_FLAG_CANDIDATES "${NOpenMP_FLAG_CANDIDATES}"
        PARENT_SCOPE)
  else()
    set(NOpenMP_${LANG}_FLAG_CANDIDATES "${NOpenMP_${LANG}_FLAG}" PARENT_SCOPE)
  endif()
endfunction()

set(NOpenMP_C_CXX_TEST_SOURCE "
#include <omp.h>
int main() {
#ifdef _OPENMP
  omp_get_max_threads();
  return 0;
#else
  breaks_on_purpose
#endif
}
")

set(NOpenMP_Fortran_TEST_SOURCE "
      program test
      @NOpenMP_Fortran_INCLUDE_LINE@
  !$  integer :: n
      n = omp_get_num_threads()
      end program test
  ")

function(_NOPENMP_WRITE_SOURCE_FILE
         LANG
         SRC_FILE_CONTENT_VAR
         SRC_FILE_NAME
         SRC_FILE_FULLPATH)
  set(WORK_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/FindNOpenMP)
  if("${LANG}" STREQUAL "C")
    set(SRC_FILE "${WORK_DIR}/${SRC_FILE_NAME}.c")
    file(WRITE "${SRC_FILE}" "${NOpenMP_C_CXX_${SRC_FILE_CONTENT_VAR}}")
  elseif("${LANG}" STREQUAL "CXX")
    set(SRC_FILE "${WORK_DIR}/${SRC_FILE_NAME}.cpp")
    file(WRITE "${SRC_FILE}" "${NOpenMP_C_CXX_${SRC_FILE_CONTENT_VAR}}")
  elseif("${LANG}" STREQUAL "Fortran")
    set(SRC_FILE "${WORK_DIR}/${SRC_FILE_NAME}.f90")
    file(WRITE "${SRC_FILE}_in" "${NOpenMP_Fortran_${SRC_FILE_CONTENT_VAR}}")
    configure_file("${SRC_FILE}_in" "${SRC_FILE}" @ONLY)
  endif()
  set(${SRC_FILE_FULLPATH} "${SRC_FILE}" PARENT_SCOPE)
endfunction()

include(CMakeParseImplicitLinkInfo)

function(_NOPENMP_GET_FLAGS
         LANG
         FLAG_MODE
         NOPENMP_FLAG_VAR
         NOPENMP_LIB_NAMES_VAR)
  _nopenmp_flag_candidates("${LANG}")
  _nopenmp_write_source_file("${LANG}"
                             "TEST_SOURCE"
                             NOpenMPTryFlag
                             _NOPENMP_TEST_SRC)

  unset(NOpenMP_VERBOSE_COMPILE_OPTIONS)
  if(WIN32)
    separate_arguments(NOpenMP_VERBOSE_OPTIONS WINDOWS_COMMAND
                       "${CMAKE_${LANG}_VERBOSE_FLAG}")
  else()
    separate_arguments(NOpenMP_VERBOSE_OPTIONS UNIX_COMMAND
                       "${CMAKE_${LANG}_VERBOSE_FLAG}")
  endif()
  foreach(_VERBOSE_OPTION IN LISTS NOpenMP_VERBOSE_OPTIONS)
    if(NOT _VERBOSE_OPTION MATCHES "^-Wl,")
      list(APPEND NOpenMP_VERBOSE_COMPILE_OPTIONS ${_VERBOSE_OPTION})
    endif()
  endforeach()

  foreach(NOPENMP_FLAG IN LISTS NOpenMP_${LANG}_FLAG_CANDIDATES)
    set(NOPENMP_FLAGS_TEST "${NOPENMP_FLAG}")
    if(NOpenMP_VERBOSE_COMPILE_OPTIONS)
      set(NOPENMP_FLAGS_TEST
          "${NOPENMP_FLAGS_TEST} ${NOpenMP_VERBOSE_COMPILE_OPTIONS}")
      # string(APPEND NOPENMP_FLAGS_TEST " ${NOpenMP_VERBOSE_COMPILE_OPTIONS}")
    endif()
    string(REGEX
           REPLACE "[-/=+]"
                   ""
                   NOPENMP_PLAIN_FLAG
                   "${NOPENMP_FLAG}")
    try_compile(NOpenMP_COMPILE_RESULT_${FLAG_MODE}_${NOPENMP_PLAIN_FLAG}
                ${CMAKE_BINARY_DIR} ${_NOPENMP_TEST_SRC}
                CMAKE_FLAGS "-DCOMPILE_DEFINITIONS:STRING=${NOPENMP_FLAGS_TEST}"
                            "${APPLE_HOMEBREW_LIBOMP}"
                LINK_LIBRARIES ${CMAKE_${LANG}_VERBOSE_FLAG}
                OUTPUT_VARIABLE NOpenMP_TRY_COMPILE_OUTPUT)

    if(NOpenMP_COMPILE_RESULT_${FLAG_MODE}_${NOPENMP_PLAIN_FLAG})
      set("${NOPENMP_FLAG_VAR}" "${NOPENMP_FLAG}" PARENT_SCOPE)

      if(CMAKE_${LANG}_VERBOSE_FLAG)
        unset(NOpenMP_${LANG}_IMPLICIT_LIBRARIES)
        unset(NOpenMP_${LANG}_IMPLICIT_LINK_DIRS)
        unset(NOpenMP_${LANG}_IMPLICIT_FWK_DIRS)
        unset(NOpenMP_${LANG}_LOG_VAR)

        file(
          APPEND
            ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
            "Detecting ${LANG} OpenMP compiler ABI info compiled with the following output:\n${NOpenMP_TRY_COMPILE_OUTPUT}\n\n"
          )

        cmake_parse_implicit_link_info("${NOpenMP_TRY_COMPILE_OUTPUT}"
                                       NOpenMP_${LANG}_IMPLICIT_LIBRARIES
                                       NOpenMP_${LANG}_IMPLICIT_LINK_DIRS
                                       NOpenMP_${LANG}_IMPLICIT_FWK_DIRS
                                       NOpenMP_${LANG}_LOG_VAR
                                       "${CMAKE_${LANG}_IMPLICIT_OBJECT_REGEX}")

        file(
          APPEND
            ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
            "Parsed ${LANG} OpenMP implicit link information from above output:\n${NOpenMP_${LANG}_LOG_VAR}\n\n"
          )

        unset(_NOPENMP_LIB_NAMES)
        foreach(_NOPENMP_IMPLICIT_LIB
                IN
                LISTS
                NOpenMP_${LANG}_IMPLICIT_LIBRARIES)
          get_filename_component(_NOPENMP_IMPLICIT_LIB_DIR
                                 "${_NOPENMP_IMPLICIT_LIB}" DIRECTORY)
          get_filename_component(_NOPENMP_IMPLICIT_LIB_NAME
                                 "${_NOPENMP_IMPLICIT_LIB}" NAME)
          get_filename_component(_NOPENMP_IMPLICIT_LIB_PLAIN
                                 "${_NOPENMP_IMPLICIT_LIB}" NAME_WE)
          string(REGEX
                 REPLACE "([][+.*?()^$])"
                         "\\\\\\1"
                         _NOPENMP_IMPLICIT_LIB_PLAIN_ESC
                         "${_NOPENMP_IMPLICIT_LIB_PLAIN}")
          string(REGEX
                 REPLACE "([][+.*?()^$])"
                         "\\\\\\1"
                         _NOPENMP_IMPLICIT_LIB_PATH_ESC
                         "${_NOPENMP_IMPLICIT_LIB}")

          list(FIND CMAKE_${LANG}_IMPLICIT_LINK_LIBRARIES
                    "${_NOPENMP_IMPLICIT_LIB}" _index)
          if(
            NOT
            (
              _index GREATER -1
              OR
                "${CMAKE_${LANG}_STANDARD_LIBRARIES}" MATCHES
                "(^| )(-Wl,)?(-l)?(${_NOPENMP_IMPLICIT_LIB_PLAIN_ESC}|${_NOPENMP_IMPLICIT_LIB_PATH_ESC})( |$)"
              OR
                "${CMAKE_${LANG}_LINK_EXECUTABLE}" MATCHES
                "(^| )(-Wl,)?(-l)?(${_NOPENMP_IMPLICIT_LIB_PLAIN_ESC}|${_NOPENMP_IMPLICIT_LIB_PATH_ESC})( |$)"
              ))
            if(_NOPENMP_IMPLICIT_LIB_DIR)
              set(
                NOpenMP_${_NOPENMP_IMPLICIT_LIB_PLAIN}_LIBRARY
                "${_NOPENMP_IMPLICIT_LIB}"
                CACHE
                  FILEPATH
                  "Path to the ${_NOPENMP_IMPLICIT_LIB_PLAIN} library for OpenMP"
                )
            else()
              find_library(
                NOpenMP_${_NOPENMP_IMPLICIT_LIB_PLAIN}_LIBRARY
                NAMES "${_NOPENMP_IMPLICIT_LIB_NAME}"
                DOC
                  "Path to the ${_NOPENMP_IMPLICIT_LIB_PLAIN} library for OpenMP"
                HINTS ${NOpenMP_${LANG}_IMPLICIT_LINK_DIRS}
                CMAKE_FIND_ROOT_PATH_BOTH NO_DEFAULT_PATH)
            endif()
            mark_as_advanced(NOpenMP_${_NOPENMP_IMPLICIT_LIB_PLAIN}_LIBRARY)
            list(APPEND _NOPENMP_LIB_NAMES ${_NOPENMP_IMPLICIT_LIB_PLAIN})
          endif()
        endforeach()
        set("${NOPENMP_LIB_NAMES_VAR}" "${_NOPENMP_LIB_NAMES}" PARENT_SCOPE)
      else()
        set("${NOPENMP_LIB_NAMES_VAR}" "" PARENT_SCOPE)
      endif()
      break()
    elseif(CMAKE_${LANG}_COMPILER_ID STREQUAL "AppleClang"
           AND CMAKE_${LANG}_COMPILER_VERSION VERSION_GREATER "6.99")
      find_library(NOpenMP_libomp_LIBRARY
                   NAMES omp gomp iomp5
                   HINTS ${CMAKE_${LANG}_IMPLICIT_LINK_DIRECTORIES})
      mark_as_advanced(NOpenMP_libomp_LIBRARY)
      if(NOpenMP_libomp_LIBRARY)
        try_compile(
          NOpenMP_COMPILE_RESULT_${FLAG_MODE}_${NOPENMP_PLAIN_FLAG}
          ${CMAKE_BINARY_DIR} ${_NOPENMP_TEST_SRC}
          CMAKE_FLAGS "-DCOMPILE_DEFINITIONS:STRING=${NOPENMP_FLAGS_TEST}"
          LINK_LIBRARIES ${CMAKE_${LANG}_VERBOSE_FLAG} ${NOpenMP_libomp_LIBRARY}
          OUTPUT_VARIABLE NOpenMP_TRY_COMPILE_OUTPUT)
        if(NOpenMP_COMPILE_RESULT_${FLAG_MODE}_${NOPENMP_PLAIN_FLAG})
          set("${NOPENMP_FLAG_VAR}" "${NOPENMP_FLAG}" PARENT_SCOPE)
          set("${NOPENMP_LIB_NAMES_VAR}" "libomp" PARENT_SCOPE)
          break()
        endif()
      endif()
    endif()
    set("${NOPENMP_LIB_NAMES_VAR}" "NOTFOUND" PARENT_SCOPE)
    set("${NOPENMP_FLAG_VAR}" "NOTFOUND" PARENT_SCOPE)
  endforeach()
  unset(NOpenMP_VERBOSE_COMPILE_OPTIONS)
endfunction()

set(NOpenMP_C_CXX_CHECK_VERSION_SOURCE "
#include <stdio.h>
#include <omp.h>
const char ompver_str[] = { 'I', 'N', 'F', 'O', ':', 'O', 'p', 'e', 'n', 'M',
                            'P', '-', 'd', 'a', 't', 'e', '[',
                            ('0' + ((_OPENMP/100000)%10)),
                            ('0' + ((_OPENMP/10000)%10)),
                            ('0' + ((_OPENMP/1000)%10)),
                            ('0' + ((_OPENMP/100)%10)),
                            ('0' + ((_OPENMP/10)%10)),
                            ('0' + ((_OPENMP/1)%10)),
                            ']', '\\0' };
int main()
{
  puts(ompver_str);
  return 0;
}
")

set(NOpenMP_Fortran_CHECK_VERSION_SOURCE "
      program omp_ver
      @NOpenMP_Fortran_INCLUDE_LINE@
      integer, parameter :: zero = ichar('0')
      integer, parameter :: ompv = openmp_version
      character, dimension(24), parameter :: ompver_str =&
      (/ 'I', 'N', 'F', 'O', ':', 'O', 'p', 'e', 'n', 'M', 'P', '-',&
         'd', 'a', 't', 'e', '[',&
         char(zero + mod(ompv/100000, 10)),&
         char(zero + mod(ompv/10000, 10)),&
         char(zero + mod(ompv/1000, 10)),&
         char(zero + mod(ompv/100, 10)),&
         char(zero + mod(ompv/10, 10)),&
         char(zero + mod(ompv/1, 10)), ']' /)
      print *, ompver_str
      end program omp_ver
")

function(_NOPENMP_GET_SPEC_DATE LANG SPEC_DATE)
  _nopenmp_write_source_file("${LANG}"
                             "CHECK_VERSION_SOURCE"
                             NOpenMPCheckVersion
                             _NOPENMP_TEST_SRC)
  set(
    BIN_FILE
    "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/FindNOpenMP/ompver_${LANG}.bin"
    )
  string(REGEX
         REPLACE "[-/=+]"
                 ""
                 NOPENMP_PLAIN_FLAG
                 "${NOPENMP_FLAG}")
  try_compile(
    NOpenMP_SPECTEST_${LANG}_${NOPENMP_PLAIN_FLAG} "${CMAKE_BINARY_DIR}"
    "${_NOPENMP_TEST_SRC}"
    CMAKE_FLAGS "-DCOMPILE_DEFINITIONS:STRING=${NOpenMP_${LANG}_FLAGS}"
    COPY_FILE ${BIN_FILE})
  if(${NOpenMP_SPECTEST_${LANG}_${NOPENMP_PLAIN_FLAG}})
    file(STRINGS ${BIN_FILE} specstr LIMIT_COUNT 1 REGEX "INFO:OpenMP-date")
    set(regex_spec_date ".*INFO:OpenMP-date\\[0*([^]]*)\\].*")
    if("${specstr}" MATCHES "${regex_spec_date}")
      set(${SPEC_DATE} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

macro(_NOPENMP_SET_VERSION_BY_SPEC_DATE LANG)
  set(NOpenMP_SPEC_DATE_MAP
      # Preview versions
      "201611=5.0" # OpenMP 5.0 preview 1
                   # Combined versions, 2.5 onwards
      "201511=4.5"
      "201307=4.0"
      "201107=3.1"
      "200805=3.0"
      "200505=2.5"
      # C/C++ version 2.0
      "200203=2.0"
      # Fortran version 2.0
      "200011=2.0"
      # Fortran version 1.1
      "199911=1.1"
      # C/C++ version 1.0 (there's no 1.1 for C/C++)
      "199810=1.0"
      # Fortran version 1.0
      "199710=1.0")

  if(NOpenMP_${LANG}_SPEC_DATE)
    string(REGEX MATCHALL
                 "${NOpenMP_${LANG}_SPEC_DATE}=([0-9]+)\\.([0-9]+)"
                 _version_match
                 "${NOpenMP_SPEC_DATE_MAP}")
  else()
    set(_version_match "")
  endif()
  if(NOT _version_match STREQUAL "")
    set(NOpenMP_${LANG}_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(NOpenMP_${LANG}_VERSION_MINOR ${CMAKE_MATCH_2})
    set(NOpenMP_${LANG}_VERSION
        "${NOpenMP_${LANG}_VERSION_MAJOR}.${NOpenMP_${LANG}_VERSION_MINOR}")
  else()
    unset(NOpenMP_${LANG}_VERSION_MAJOR)
    unset(NOpenMP_${LANG}_VERSION_MINOR)
    unset(NOpenMP_${LANG}_VERSION)
  endif()
  unset(_version_match)
  unset(NOpenMP_SPEC_DATE_MAP)
endmacro()

foreach(LANG IN ITEMS C CXX)
  if(CMAKE_${LANG}_COMPILER_LOADED)
    if(NOT DEFINED NOpenMP_${LANG}_FLAGS
       OR "${NOpenMP_${LANG}_FLAGS}" STREQUAL "NOTFOUND"
       OR NOT DEFINED NOpenMP_${LANG}_LIB_NAMES
       OR "${NOpenMP_${LANG}_LIB_NAMES}" STREQUAL "NOTFOUND")
      _nopenmp_get_flags("${LANG}"
                         "${LANG}"
                         NOpenMP_${LANG}_FLAGS_WORK
                         NOpenMP_${LANG}_LIB_NAMES_WORK)
    endif()
    set(NOpenMP_${LANG}_FLAGS "${NOpenMP_${LANG}_FLAGS_WORK}"
        CACHE STRING "${LANG} compiler flags for OpenMP parallelization")
    set(NOpenMP_${LANG}_LIB_NAMES "${NOpenMP_${LANG}_LIB_NAMES_WORK}"
        CACHE STRING "${LANG} compiler libraries for OpenMP parallelization")
    mark_as_advanced(NOpenMP_${LANG}_FLAGS NOpenMP_${LANG}_LIB_NAMES)
  endif()
endforeach()

if(CMAKE_Fortran_COMPILER_LOADED)
  if(NOT DEFINED NOpenMP_Fortran_FLAGS
     OR "${NOpenMP_Fortran_FLAGS}" STREQUAL "NOTFOUND"
     OR NOT DEFINED NOpenMP_Fortran_LIB_NAMES
     OR "${NOpenMP_Fortran_LIB_NAMES}" STREQUAL "NOTFOUND"
     OR NOT DEFINED NOpenMP_Fortran_HAVE_OMPLIB_MODULE)
    set(NOpenMP_Fortran_INCLUDE_LINE "use omp_lib\n      implicit none")
    _nopenmp_get_flags("Fortran"
                       "FortranHeader"
                       NOpenMP_Fortran_FLAGS_WORK
                       NOpenMP_Fortran_LIB_NAMES_WORK)
    if(NOpenMP_Fortran_FLAGS_WORK)
      set(NOpenMP_Fortran_HAVE_OMPLIB_MODULE TRUE CACHE BOOL INTERNAL "")
    endif()
    set(NOpenMP_Fortran_FLAGS "${NOpenMP_Fortran_FLAGS_WORK}"
        CACHE STRING "Fortran compiler flags for OpenMP parallelization")
    set(NOpenMP_Fortran_LIB_NAMES "${NOpenMP_Fortran_LIB_NAMES_WORK}"
        CACHE STRING "Fortran compiler libraries for OpenMP parallelization")
    mark_as_advanced(NOpenMP_Fortran_FLAGS NOpenMP_Fortran_LIB_NAMES)
  endif()

  if(NOT DEFINED NOpenMP_Fortran_FLAGS
     OR "${NOpenMP_Fortran_FLAGS}" STREQUAL "NOTFOUND"
     OR NOT DEFINED NOpenMP_Fortran_LIB_NAMES
     OR "${NOpenMP_Fortran_LIB_NAMES}" STREQUAL "NOTFOUND"
     OR NOT DEFINED NOpenMP_Fortran_HAVE_OMPLIB_HEADER)
    set(NOpenMP_Fortran_INCLUDE_LINE "implicit none\n      include 'omp_lib.h'")
    _nopenmp_get_flags("Fortran"
                       "FortranModule"
                       NOpenMP_Fortran_FLAGS_WORK
                       NOpenMP_Fortran_LIB_NAMES_WORK)
    if(NOpenMP_Fortran_FLAGS_WORK)
      set(NOpenMP_Fortran_HAVE_OMPLIB_HEADER TRUE CACHE BOOL INTERNAL "")
    endif()
    set(NOpenMP_Fortran_FLAGS "${NOpenMP_Fortran_FLAGS_WORK}"
        CACHE STRING "Fortran compiler flags for OpenMP parallelization")

    set(NOpenMP_Fortran_LIB_NAMES "${NOpenMP_Fortran_LIB_NAMES}"
        CACHE STRING "Fortran compiler libraries for OpenMP parallelization")
  endif()
  if(NOpenMP_Fortran_HAVE_OMPLIB_MODULE)
    set(NOpenMP_Fortran_INCLUDE_LINE "use omp_lib\n      implicit none")
  else()
    set(NOpenMP_Fortran_INCLUDE_LINE "implicit none\n      include 'omp_lib.h'")
  endif()
endif()

if(NOT NOpenMP_FIND_COMPONENTS)
  set(NOpenMP_FINDLIST C CXX Fortran)
else()
  set(NOpenMP_FINDLIST ${NOpenMP_FIND_COMPONENTS})
endif()

unset(_NOpenMP_MIN_VERSION)

include(FindPackageHandleStandardArgs)

foreach(LANG IN LISTS NOpenMP_FINDLIST)
  if(CMAKE_${LANG}_COMPILER_LOADED)
    if(NOT NOpenMP_${LANG}_SPEC_DATE AND NOpenMP_${LANG}_FLAGS)
      _nopenmp_get_spec_date("${LANG}" NOpenMP_${LANG}_SPEC_DATE_INTERNAL)
      set(NOpenMP_${LANG}_SPEC_DATE "${NOpenMP_${LANG}_SPEC_DATE_INTERNAL}"
          CACHE INTERNAL "${LANG} compiler's OpenMP specification date")
      _nopenmp_set_version_by_spec_date("${LANG}")
    endif()

    set(NOpenMP_${LANG}_FIND_QUIETLY ${NOpenMP_FIND_QUIETLY})
    set(NOpenMP_${LANG}_FIND_REQUIRED ${NOpenMP_FIND_REQUIRED})
    set(NOpenMP_${LANG}_FIND_VERSION ${NOpenMP_FIND_VERSION})
    set(NOpenMP_${LANG}_FIND_VERSION_EXACT ${NOpenMP_FIND_VERSION_EXACT})

    set(_NOPENMP_${LANG}_REQUIRED_VARS NOpenMP_${LANG}_FLAGS)
    if("${NOpenMP_${LANG}_LIB_NAMES}" STREQUAL "NOTFOUND")
      set(_NOPENMP_${LANG}_REQUIRED_LIB_VARS NOpenMP_${LANG}_LIB_NAMES)
    else()
      foreach(_NOPENMP_IMPLICIT_LIB IN LISTS NOpenMP_${LANG}_LIB_NAMES)
        list(APPEND _NOPENMP_${LANG}_REQUIRED_LIB_VARS
                    NOpenMP_${_NOPENMP_IMPLICIT_LIB}_LIBRARY)
      endforeach()
    endif()

    find_package_handle_standard_args(NOpenMP_${LANG}
                                      FOUND_VAR
                                      NOpenMP_${LANG}_FOUND
                                      REQUIRED_VARS
                                      NOpenMP_${LANG}_FLAGS
                                      ${_NOPENMP_${LANG}_REQUIRED_LIB_VARS}
                                      VERSION_VAR
                                      NOpenMP_${LANG}_VERSION)

    if(NOpenMP_${LANG}_FOUND)
      if(DEFINED NOpenMP_${LANG}_VERSION)
        if(NOT _NOpenMP_MIN_VERSION
           OR _NOpenMP_MIN_VERSION VERSION_GREATER NOpenMP_${LANG}_VERSION)
          set(_NOpenMP_MIN_VERSION NOpenMP_${LANG}_VERSION)
        endif()
      endif()
      set(NOpenMP_${LANG}_LIBRARIES "")
      foreach(_NOPENMP_IMPLICIT_LIB IN LISTS NOpenMP_${LANG}_LIB_NAMES)
        list(APPEND NOpenMP_${LANG}_LIBRARIES
                    "${NOpenMP_${_NOPENMP_IMPLICIT_LIB}_LIBRARY}")
      endforeach()

      if(NOT TARGET OpenMP::OpenMP_${LANG})
        if(CMAKE_VERSION VERSION_LESS 3.0)
          add_library(OpenMP::OpenMP_${LANG} UNKNOWN IMPORTED)
          list(GET NOpenMP_${LANG}_LIB_NAMES 0 _name)
          set_target_properties(OpenMP::OpenMP_${LANG}
                                PROPERTIES IMPORTED_LOCATION
                                           "${NOpenMP_${_name}_LIBRARY}")
        else()
          add_library(OpenMP::OpenMP_${LANG} INTERFACE IMPORTED)
        endif()
      endif()
      if(NOpenMP_${LANG}_FLAGS)
        if(WIN32)
          separate_arguments(_NOpenMP_${LANG}_OPTIONS WINDOWS_COMMAND
                             "${NOpenMP_${LANG}_FLAGS}")
        else()
          separate_arguments(_NOpenMP_${LANG}_OPTIONS UNIX_COMMAND
                             "${NOpenMP_${LANG}_FLAGS}")
        endif()
        if(CMAKE_VERSION VERSION_LESS 3.3)
          set_property(TARGET OpenMP::OpenMP_${LANG}
                       PROPERTY INTERFACE_COMPILE_OPTIONS
                                "${_NOpenMP_${LANG}_OPTIONS}")
        else()
          set_property(
            TARGET OpenMP::OpenMP_${LANG}
            PROPERTY
              INTERFACE_COMPILE_OPTIONS
              "$<$<COMPILE_LANGUAGE:${LANG}>:${_NOpenMP_${LANG}_OPTIONS}>")
        endif()
        unset(_NOpenMP_${LANG}_OPTIONS)
      endif()
      if(NOpenMP_${LANG}_LIBRARIES)
        set_property(TARGET OpenMP::OpenMP_${LANG}
                     PROPERTY INTERFACE_LINK_LIBRARIES
                              "${NOpenMP_${LANG}_LIBRARIES}")
      endif()
    endif()
  endif()
endforeach()

unset(_NOpenMP_REQ_VARS)
foreach(LANG IN ITEMS C CXX Fortran)
  list(FIND NOpenMP_FIND_COMPONENTS "${LANG}" _index)

  if((NOT NOpenMP_FIND_COMPONENTS AND CMAKE_${LANG}_COMPILER_LOADED)
     OR _index GREATER -1)
    list(APPEND _NOpenMP_REQ_VARS "NOpenMP_${LANG}_FOUND")
  endif()
endforeach()

find_package_handle_standard_args(NOpenMP
                                  FOUND_VAR
                                  NOpenMP_FOUND
                                  REQUIRED_VARS
                                  ${_NOpenMP_REQ_VARS}
                                  VERSION_VAR
                                  ${_NOpenMP_MIN_VERSION}
                                  HANDLE_COMPONENTS)

set(NOPENMP_FOUND ${NOpenMP_FOUND})

if(CMAKE_Fortran_COMPILER_LOADED AND NOpenMP_Fortran_FOUND)
  if(NOT DEFINED NOpenMP_Fortran_HAVE_OMPLIB_MODULE)
    set(NOpenMP_Fortran_HAVE_OMPLIB_MODULE FALSE CACHE BOOL INTERNAL "")
  endif()
  if(NOT DEFINED NOpenMP_Fortran_HAVE_OMPLIB_HEADER)
    set(NOpenMP_Fortran_HAVE_OMPLIB_HEADER FALSE CACHE BOOL INTERNAL "")
  endif()
endif()

if(NOT
   (CMAKE_C_COMPILER_LOADED
    OR CMAKE_CXX_COMPILER_LOADED
    OR CMAKE_Fortran_COMPILER_LOADED))
  message(
    SEND_ERROR
      "FindNOpenMP requires the C, CXX or Fortran languages to be enabled")
endif()

unset(NOpenMP_C_CXX_TEST_SOURCE)
unset(NOpenMP_Fortran_TEST_SOURCE)
unset(NOpenMP_C_CXX_CHECK_VERSION_SOURCE)
unset(NOpenMP_Fortran_CHECK_VERSION_SOURCE)
unset(NOpenMP_Fortran_INCLUDE_LINE)
