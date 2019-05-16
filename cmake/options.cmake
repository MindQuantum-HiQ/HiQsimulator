include(CMakeDependentOption)

# ------------------------------------------------------------------------------

option(USE_INTRIN "Enable/disable the use of intrinsics" ON)
cmake_dependent_option(USE_INTRIN_BUFFER
                       "Using intrinsics with buffer"
                       OFF
                       "USE_INTRIN"
                       OFF)

option(USE_CLANG_FORMAT "Setup clangformat target" OFF)

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

# ==============================================================================

if(USE_CLANG_FORMAT)
  include(ClangFormat)

  file(GLOB main_srcs LIST_DIRECTORIES FALSE "*.h" "*.hpp" "*.cpp")
  file(GLOB_RECURSE src_srcs
       LIST_DIRECTORIES FALSE
       "${PROJECT_SOURCE_DIR}/src/*.h" "${PROJECT_SOURCE_DIR}/src/*.hpp"
       "${PROJECT_SOURCE_DIR}/src/*.cpp")
  clangformat_setup("${main_srcs};${src_srcs}")
endif()
