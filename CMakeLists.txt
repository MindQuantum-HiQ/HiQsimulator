cmake_minimum_required(VERSION 3.5)

project(HiQSimulator)

include(CheckIncludeFileCXX)

message("*** Building ${CMAKE_BUILD_TYPE} configuration ***")

IF(USE_INTRIN STREQUAL "BUF")
  ADD_DEFINITIONS(-DINTRIN_CF)
  MESSAGE("Using intrin with buffer")
ELSEIF(USE_INTRIN STREQUAL "NO")
  ADD_DEFINITIONS(-DNOINTRIN)
  MESSAGE("Using no intrinsics")
ELSE(USE_INTRIN)
  ADD_DEFINITIONS(-DINTRIN)
  MESSAGE("Using default intrin")
ENDIF(USE_INTRIN STREQUAL "BUF")

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

find_package(Boost REQUIRED
        COMPONENTS program_options mpi serialization thread
        )

find_library(Glog_LIBRARY glog PATHS ~/.nix-profile/lib)

find_library(HWLOC_LIBRARY hwloc PATHS ~/.nix-profile/lib)

set(CMAKE_CXX_FLAGS "-std=c++11 -fopenmp -fPIC -fpic  -Wall -Wextra -pedantic -pie -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

get_directory_property(defines COMPILE_DEFINITIONS)
message("*** common definitions (-D): ${defines}")
message("*** CMAKE_CXX_FLAGS        : ${CMAKE_CXX_FLAGS}")
message("*** CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")

set(FLAGS_MPI "")
set(CMAKE_CXX_FLAGS_DEBUG_MPI "${CMAKE_CXX_FLAGS_DEBUG} ${MPI_COMPILE_FLAGS} ${FLAGS_MPI}")
set(CMAKE_CXX_FLAGS_RELEASE_MPI "${CMAKE_CXX_FLAGS_RELEASE} ${MPI_COMPILE_FLAGS} ${FLAGS_MPI}")

set(CMAKE_LINK_FLAGS_DEBUG_MPI "${MPI_LINK_FLAGS}")
set(CMAKE_LINK_FLAGS_RELEASE_MPI "${MPI_LINK_FLAGS}")

add_subdirectory(pybind11)

add_subdirectory("src")

include_directories("src")
include_directories(${Boost_INCLUDE_DIR})

add_executable(qswap qswap.cpp)
target_link_libraries(qswap permutations ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE})

add_executable(opyt opyt.cpp)
target_link_libraries(opyt ${Boost_LIBRARIES})

add_executable(socket-test socket-test.cpp)
target_link_libraries(socket-test ${Boost_LIBRARIES})

add_executable(himembind himembind.cpp)
target_link_libraries(himembind ${HWLOC_LIBRARY})

pybind11_add_module(_sched_cpp _sched_cpp.cpp
        src/scheduler/swap_scheduler.h
        src/scheduler/cluster_scheduler.h
        src/scheduler/convertors.h
        src/scheduler/definitions.h
        src/scheduler/swap_scheduler.cpp
        src/scheduler/cluster_scheduler.cpp
        src/scheduler/convertors.cpp
        )

target_link_libraries(_sched_cpp PUBLIC
        ${Glog_LIBRARY}
        )

add_executable(qswap-mpi
        qswap-mpi.cpp
        src/simulator-mpi/SwapperMT.hpp
        src/simulator-mpi/SwapperMT.cpp
        )
target_link_libraries(qswap-mpi permutations
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        ${Glog_LIBRARY}
        )


add_library(SimulatorMPI_o OBJECT
        src/simulator-mpi/SwapperMT.hpp
        src/simulator-mpi/SwapperMT.cpp
        src/simulator-mpi/swapping.hpp
        src/simulator-mpi/SimulatorMPI.hpp
        src/simulator-mpi/SimulatorMPI.cpp
        src/simulator-mpi/fusion_mpi.hpp
        src/simulator-mpi/alignedallocator.hpp

        src/simulator-mpi/kernels/intrin/cintrin.hpp
        src/simulator-mpi/kernels/intrin/kernel3.hpp
        src/simulator-mpi/kernels/intrin/kernel2.hpp
        src/simulator-mpi/kernels/intrin/kernel5.hpp
        src/simulator-mpi/kernels/intrin/kernels.hpp
        src/simulator-mpi/kernels/intrin/kernel1.hpp
        src/simulator-mpi/kernels/intrin/kernel4.hpp
        src/simulator-mpi/kernels/intrin/kernels_diag.hpp

        src/simulator-mpi/kernels/intrin_cf/kernel3.hpp
        src/simulator-mpi/kernels/intrin_cf/kernel2.hpp
        src/simulator-mpi/kernels/intrin_cf/kernel5.hpp
        src/simulator-mpi/kernels/intrin_cf/kernels.hpp
        src/simulator-mpi/kernels/intrin_cf/kernel1.hpp
        src/simulator-mpi/kernels/intrin_cf/kernel4.hpp

        src/simulator-mpi/kernels/nointrin/kernel3.hpp
        src/simulator-mpi/kernels/nointrin/kernel2.hpp
        src/simulator-mpi/kernels/nointrin/kernel5.hpp
        src/simulator-mpi/kernels/nointrin/kernels.hpp
        src/simulator-mpi/kernels/nointrin/kernel1.hpp
        src/simulator-mpi/kernels/nointrin/kernel4.hpp
        src/simulator-mpi/kernels/nointrin/kernels_diag.hpp

        )

add_library(SimulatorMPI SHARED $<TARGET_OBJECTS:SimulatorMPI_o>)
target_link_libraries(SimulatorMPI
        permutations
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        ${Glog_LIBRARY}
        )

add_executable(cppsim-mpi cppsim-mpi.cpp
        src/simulator-mpi/SimulatorMPI.hpp
        )

target_link_libraries(cppsim-mpi
        SimulatorMPI
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        ${Glog_LIBRARY}
        )

add_executable(opyt-mpi opyt-mpi.cpp)
target_link_libraries(opyt-mpi
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        )


pybind11_add_module(_cppsim_mpi _cppsim_mpi.cpp $<TARGET_OBJECTS:SimulatorMPI_o>)
target_link_libraries(_cppsim_mpi PUBLIC
        ${MPI_LIBRARIES}
        ${Boost_MPI_LIBRARY_RELEASE}
        ${Boost_THREAD_LIBRARY_RELEASE}
        ${Boost_SERIALIZATION_LIBRARY_RELEASE}
        ${Glog_LIBRARY}
        )

pybind11_add_module(_cppstabsim _cppstabsim.cpp)
target_compile_options(_cppstabsim PUBLIC "-mavx2")
