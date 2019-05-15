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

  file(GLOB_RECURSE clangformat_srcs LIST_DIRECTORIES FALSE "*.hpp" "*.cpp")
  clangformat_setup("${clangformat_srcs}")
endif()
