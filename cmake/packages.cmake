# ==============================================================================
# OpenMP

if(APPLE)
  find_program(BREW_CMD brew PATHS /usr/local/bin)
  if(BREW_CMD)
    execute_process(COMMAND ${BREW_CMD} --prefix llvm
                    OUTPUT_VARIABLE LLVM_PREFIX)
    string(STRIP ${LLVM_PREFIX} LLVM_PREFIX)
    set(APPLE_HOMEBREW_LIBOMP "-DLINK_DIRECTORIES:STRING=${LLVM_PREFIX}/lib")
  endif()
endif()

if(CMAKE_VERSION VERSION_LESS 3.11 OR APPLE)
  # Actually CMake 3.9 should be enough for OpenMP::OpenMP support,
  # however, until CMake 3.11 we might get problems with the
  # INTERFACE_COMPILE_OPTIONS property if we are compiling with
  # CMake's native CUDA compilation
  # On Mac using Homebrew the libomp library might not be in the path, so
  # we need to use the custom find
  find_package(NOpenMP REQUIRED) # NOpenMP is FindOpenMP.cmake from 3.12
else()
  find_package(OpenMP REQUIRED)
endif()

# ==============================================================================

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# ==============================================================================

find_package(Boost REQUIRED
             COMPONENTS program_options
                        mpi
                        serialization
                        thread)

# ==============================================================================

find_package(glog REQUIRED)

# ==============================================================================

find_package(hwloc REQUIRED)

# ==============================================================================

