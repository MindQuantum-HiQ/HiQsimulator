include(CMakeDependentOption)

# ------------------------------------------------------------------------------

option(USE_CLANG_FORMAT "Setup clangformat target" OFF)

# ------------------------------------------------------------------------------

option(USE_INTRIN "Enable/disable the use of intrinsics" ON)
cmake_dependent_option(USE_INTRIN_BUFFER
                       "Using intrinsics with buffer"
                       OFF
                       "USE_INTRIN"
                       OFF)

# ------------------------------------------------------------------------------

option(PYBIND11_NO_THROW_RUNTIME_ERROR "(for stabilizer simulator only) \
Do not throw std::runtime_error exceptions, instead use the usual \
Python C API exception mechanism" OFF)

# ------------------------------------------------------------------------------

option(USE_SA "Turn on static analysis during compiling and linking" OFF)
cmake_dependent_option(USE_SA_CPPCHECK
                       "Run cppcheck on each file"
                       OFF
                       "USE_SA"
                       OFF)
cmake_dependent_option(USE_SA_CLANG_TIDY
                       "Run clang-tidy on each file"
                       OFF
                       "USE_SA"
                       OFF)
cmake_dependent_option(USE_SA_IWYU
                       "Run include-what-you-use on each file"
                       OFF
                       "USE_SA"
                       OFF)
cmake_dependent_option(USE_SA_LWYU
                       "Run link-what-you-use at the linking stage"
                       OFF
                       "USE_SA"
                       OFF)

# ------------------------------------------------------------------------------

option(HIQ_TEST "Build the HiQSimulator test suite?" OFF)
cmake_dependent_option(HIQ_TEST_XML_OUTPUT
                       "Use the XML format to output the results of \
unit testing (useful for CI)"
                       OFF
                       "HIQ_TEST"
                       OFF)

# ==============================================================================

if(USE_INTRIN)
  if(USE_INTRIN_BUFFER)
    add_definitions(-DINTRIN_CF)
  else()
    add_definitions(-DINTRIN)
  endif()
else(USE_INTRIN)
  add_definitions(-DNOINTRIN)
endif()

# ------------------------------------------------------------------------------

if(USE_CLANG_FORMAT)
  include(ClangFormat)

  file(GLOB main_srcs
            LIST_DIRECTORIES
            FALSE
            "*.h"
            "*.hpp"
            "*.cpp")
  file(GLOB_RECURSE src_srcs
                    LIST_DIRECTORIES
                    FALSE
                    "${PROJECT_SOURCE_DIR}/src/*.h"
                    "${PROJECT_SOURCE_DIR}/src/*.hpp"
                    "${PROJECT_SOURCE_DIR}/src/*.cpp")
  clangformat_setup("${main_srcs};${src_srcs}")
endif()

# ------------------------------------------------------------------------------

if(PYBIND11_NO_THROW_RUNTIME_ERROR)
  add_definitions(-DPYBIND11_NO_THROW_RUNTIME_ERROR)
endif()

# ------------------------------------------------------------------------------

if(USE_SA)
  if(USE_SA_CPPCHECK)
    find_program(_cppcheck NAMES cppcheck DOC "cppcheck executable path")
    mark_as_advanced(_cppcheck)
    if(NOT _cppcheck)
      message(WARNING "Unable to find the path to the cppcheck executable")
    else()
      set(USE_SA_CPPCHECK_CXX_ARGS "--std=c++14;--enable=all"
          CACHE STRING "Arguments to pass to cppcheck for C++ code")
      set(CMAKE_CXX_CPPCHECK "${_cppcheck};${USE_SA_CPPCHECK_CXX_ARGS}")
    endif()
  endif()

  if(USE_SA_CLANG_TIDY)
    find_program(_clang_tidy NAMES clang-tidy DOC "clang-tidy executable path")
    mark_as_advanced(_clang_tidy)
    if(NOT _clang_tidy)
      message(WARNING "Unable to find the path to the clang-tidy executable")
    else()
      set(USE_SA_CLANG_TIDY_CXX_ARGS "-checks=*"
          CACHE STRING "Arguments to pass to clang-tidy for C++ code")
      set(CMAKE_CXX_CLANG_TIDY "${_clang_tidy};${USE_SA_CLANG_TIDY_CXX_ARGS}")
    endif()
  endif()

  if(USE_SA_IWYU)
    find_program(_iwyu
                 NAMES iwyu include-what-you-use
                 DOC "include-what-you-use executable path")
    mark_as_advanced(_iwyu)
    if(NOT _iwyu)
      message(WARNING "Unable to find the path to the _iwyu executable")
    else()
      set(USE_SA_IWYU_CXX_ARGS ""
          CACHE STRING "Arguments to pass to include-what-you-use for C++ code")
      set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${_iwyu};${USE_SA_IWYU_CXX_ARGS}")
    endif()
  endif()
  if(USE_SA_LWYU)
    set(CMAKE_LINK_WHAT_YOU_USE TRUE)
  endif()
endif()

# ------------------------------------------------------------------------------
