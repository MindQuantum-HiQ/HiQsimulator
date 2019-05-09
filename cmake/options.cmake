include(CMakeDependentOption)

# ------------------------------------------------------------------------------

option(USE_INTRIN "Enable/disable the use of intrinsics" ON)
cmake_dependent_option(USE_INTRIN_BUFFER "Using intrinsics with buffer" OFF
  "USE_INTRIN" OFF)

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
