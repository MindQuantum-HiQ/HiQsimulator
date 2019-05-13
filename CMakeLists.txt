cmake_minimum_required(VERSION 3.5)

set(_policy_list
    CMP0012
    CMP0023
    CMP0028
    CMP0042
    CMP0048
    CMP0051
    CMP0054
    CMP0057
    CMP0074)
foreach(_policy ${_policy_list})
  if(POLICY ${_policy})
    cmake_policy(SET ${_policy} NEW)
  endif()
  # CMP0012: if() recognizes numbers and booleans
  # CMP0028: :: in target names
  # CMP0042: MACOS_RPATH
  # CMP0048: allow VERSION in project()
  # CMP0051: list TARGET_OBJECTS in SOURCES property
  # CMP0054: no more de-referencing of "expr" in if() statements
  # CMP0057: if IN_LIST
  # CMP0074: XXX_ROOT variables for find_package(XXX)
endforeach()

# ==============================================================================
# Macro definitions

include(${CMAKE_CURRENT_LIST_DIR}/cmake/macros.cmake)

# ==============================================================================
# Create the HiQSimulator project

project(HiQSimulator LANGUAGES CXX)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE
               PROPERTY STRINGS
                        "Debug"
                        "Release"
                        "RelWithDebInfo")
endif()

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)

#==============================================================================
# Options

include(${CMAKE_CURRENT_LIST_DIR}/cmake/options.cmake)

# ==============================================================================
# Package dependencies

include(${CMAKE_CURRENT_LIST_DIR}/cmake/packages.cmake)

#==============================================================================
# Setup compiler flags

set(CMAKE_CXX_STANDARD 14) # might decay to C++11
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

check_compiler_flags(_compile_flags_release
                     "-ffast-math /fp:fast -fast"
                     "-O3 /Ox"
                     "-march=native -xHost /QxHost")

add_compile_options(
  "$<$<AND:$<CONFIG:RELEASE>,$<COMPILE_LANGUAGE:CXX>>:${_compile_flags_release_cxx}>"
  )

check_compiler_flags(_avx2_flag "-mavx2 -xCORE-AVX2 /QxCORE-AVX2 /arch:AVX2")

# ------------------------------------------------------------------------------

set(FLAGS_MPI)
set(CMAKE_CXX_FLAGS_DEBUG_MPI
    "${CMAKE_CXX_FLAGS_DEBUG} ${MPI_COMPILE_FLAGS} ${FLAGS_MPI}")
set(CMAKE_CXX_FLAGS_RELEASE_MPI
    "${CMAKE_CXX_FLAGS_RELEASE} ${MPI_COMPILE_FLAGS} ${FLAGS_MPI}")

set(CMAKE_LINK_FLAGS_DEBUG_MPI "${MPI_LINK_FLAGS}")
set(CMAKE_LINK_FLAGS_RELEASE_MPI "${MPI_LINK_FLAGS}")

# ------------------------------------------------------------------------------

set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd")
set(CMAKE_MINSIZEREL_POSTFIX "rm")

# ==============================================================================

message("*** Building ${CMAKE_BUILD_TYPE} configuration ***")

add_subdirectory(pybind11)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)

# ==============================================================================

add_executable(qswap qswap.cpp)
target_link_libraries(qswap permutations Boost::program_options)

# --------------------------------------

add_executable(qswap-mpi qswap-mpi.cpp src/simulator-mpi/SwapperMT.cpp)
target_compile_definitions(qswap-mpi PRIVATE -DSIMULATOR_LIBRARY_EXPORT)
target_link_libraries(qswap-mpi
                      permutations
                      ${MPI_LIBRARIES}
                      Boost::program_options
                      Boost::mpi
                      Boost::serialization
                      Boost::thread
                      OpenMP::OpenMP_CXX)

# ------------------------------------------------------------------------------

add_executable(opyt opyt.cpp)
target_link_libraries(opyt
                      Boost::program_options
                      Boost::mpi
                      Boost::serialization
                      Boost::thread
                      OpenMP::OpenMP_CXX
                      glog::glog)

# --------------------------------------

add_executable(opyt-mpi opyt-mpi.cpp)
target_link_libraries(opyt-mpi
                      ${MPI_LIBRARIES}
                      Boost::program_options
                      Boost::mpi
                      Boost::serialization
                      Boost::thread
                      OpenMP::OpenMP_CXX
                      glog::glog)

# ------------------------------------------------------------------------------

add_executable(socket-test socket-test.cpp)
target_link_libraries(socket-test
                      Boost::program_options
                      Boost::mpi
                      Boost::serialization
                      Boost::thread
                      OpenMP::OpenMP_CXX)

# ------------------------------------------------------------------------------

add_executable(himembind himembind.cpp)
target_link_libraries(himembind hwloc::hwloc)

# ------------------------------------------------------------------------------

pybind11_add_module(_sched_cpp
                    _sched_cpp.cpp
                    src/scheduler/swap_scheduler.cpp
                    src/scheduler/cluster_scheduler.cpp
                    src/scheduler/convertors.cpp)
target_link_libraries(_sched_cpp PUBLIC glog::glog)

# ------------------------------------------------------------------------------

add_library(SimulatorMPI_o
            OBJECT
            src/simulator-mpi/SwapperMT.cpp
            src/simulator-mpi/SimulatorMPI.cpp)
target_link_libraries(SimulatorMPI_o permutations Boost::mpi OpenMP::OpenMP_CXX)
if(WIN32)
  target_compile_definitions(SimulatorMPI_o PRIVATE -DSIMULATOR_LIBRARY_EXPORT)
endif()

add_library(SimulatorMPI SHARED $<TARGET_OBJECTS:SimulatorMPI_o>)
target_link_libraries(SimulatorMPI
                      permutations
                      ${MPI_LIBRARIES}
                      Boost::program_options
                      Boost::mpi
                      Boost::serialization
                      Boost::thread
                      OpenMP::OpenMP_CXX)

# ------------------------------------------------------------------------------

add_executable(cppsim-mpi cppsim-mpi.cpp)
target_link_libraries(cppsim-mpi
                      SimulatorMPI
                      ${MPI_LIBRARIES}
                      Boost::program_options
                      Boost::mpi
                      Boost::serialization
                      Boost::thread
                      glog::glog
                      OpenMP::OpenMP_CXX)

# ------------------------------------------------------------------------------

pybind11_add_module(_cppsim_mpi _cppsim_mpi.cpp
                    $<TARGET_OBJECTS:SimulatorMPI_o>)
target_link_libraries(_cppsim_mpi
                      PUBLIC ${MPI_LIBRARIES}
                             Boost::mpi
                             Boost::serialization
                             Boost::thread
                             glog::glog
                             OpenMP::OpenMP_CXX)

# ------------------------------------------------------------------------------

pybind11_add_module(_cppstabsim _cppstabsim.cpp)
target_link_libraries(_cppstabsim PRIVATE Boost::boost)
add_avx2_to_target(_cppstabsim)

# ==============================================================================
