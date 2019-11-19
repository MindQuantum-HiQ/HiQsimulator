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
  set(_openmp_name NOpenMP)
  find_package(NOpenMP) # NOpenMP is FindOpenMP.cmake from 3.12
else()
  set(_openmp_name OpenMP)
  find_package(OpenMP)
endif()

if(${_openmp_name}_FOUND)
  set(OpenMP_tgt OpenMP::OpenMP_CXX)
endif()

# ==============================================================================

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# ==============================================================================

find_package(Boost 1.55 REQUIRED
             COMPONENTS program_options
                        mpi
                        serialization
                        thread)

get_target_property(_headers_path Boost::boost INTERFACE_INCLUDE_DIRECTORIES)
if(NOT _header_path)
  if(TARGET Boost::headers)
    get_target_property(_tmp Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
    set_property(TARGET Boost::boost
                 APPEND
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${_tmp})
  else()
    get_target_property(_tmp Boost::program_options
                        INTERFACE_INCLUDE_DIRECTORIES)
    set_property(TARGET Boost::boost
                 APPEND
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${_tmp})
  endif()
endif()

if(Boost_MINOR_VERSION LESS 59)
  set(BUILD_TESTING OFF CACHE BOOL "Build the HiQSimulator test suite?" FORCE)
  message(WARNING "Unable to compile unit tests! Boost version is too old \
(${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION} vs. 1.59). \
Testing disabled.")
endif()

if(BUILD_TESTING)
  # Boost 1.59 required for Boost.Test v3
  find_package(Boost 1.59 REQUIRED COMPONENTS unit_test_framework)

  if(CMAKE_VERSION VERSION_LESS 3.12)
    find_package(BPython 3.0 REQUIRED COMPONENTS Interpreter)
  else()
    find_package(Python 3.0 REQUIRED COMPONENTS Interpreter)
  endif()

  message(STATUS "Found Python ${Python_VERSION}:")
  message(STATUS "  - Interpreter (${Python_INTERPRETER_ID}): "
                 "${Python_EXECUTABLE}")
  message(STATUS "  - Development:")
  message(STATUS "    + site-lib: ${Python_SITELIB}")

  execute_process(COMMAND ${Python_EXECUTABLE} -c
                          "from projectq.backends import Simulator"
                  RESULT_VARIABLE _found_projectq
                  ERROR_VARIABLE _import_projectq_err
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT _found_projectq EQUAL 0)
    message(WARNING "Unable to import the ProjectQ Python module. "
                    "\nMay have to skip some unit tests.")
  else()
    set(_found_projectq TRUE)
  endif()
endif()
# ------------------------------------------------------------------------------
# In some cases where the Boost version is too recent compared to what the
# CMake version supports, the imported target are not properly defined.
# In this case, try our best to define the target properly and warn the user

if(Boost_FOUND)
  set(_BOOST_HAS_IMPORTED_TGT TRUE)

  if(NOT
     Boost_VERSION
     VERSION_LESS
     105300
     AND Boost_VERSION VERSION_LESS 105600)
    set(Boost_ATOMIC_DEPENDENCIES
        thread
        chrono
        system
        date_time)
  endif()
  set(Boost_CHRONO_DEPENDENCIES system)
  set(Boost_MPI_DEPENDENCIES serialization)
  if(NOT Boost_VERSION VERSION_LESS 105300)
    set(Boost_THREAD_DEPENDENCIES
        chrono
        system
        date_time
        atomic)
  elseif(NOT Boost_VERSION VERSION_LESS 105000)
    set(Boost_THREAD_DEPENDENCIES chrono system date_time)
  else()
    set(Boost_THREAD_DEPENDENCIES date_time)
  endif()

  foreach(comp ${_Boost_COMPONENTS_SEARCHED})
    if(NOT TARGET Boost::${comp})
      set(_BOOST_HAS_IMPORTED_TGT FALSE)
      define_target(Boost ${comp} "${Boost_INCLUDE_DIR}")
    endif()
  endforeach()

  if(NOT _BOOST_HAS_IMPORTED_TGT)
    message(
      WARNING
        "Your version of Boost (${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}."
        "${Boost_SUBMINOR_VERSION}) is too recent for this version"
        " of CMake."
        "\nThe imported target have been defined manually, but the dependencies "
        " might not be correct. If compilation fails, please try updating your"
        " CMake installation to a newer version (for your information, your "
        "CMake version is ${CMAKE_VERSION}).")
  endif()
endif()

# ==============================================================================

find_package(glog REQUIRED)

# ==============================================================================

find_package(hwloc REQUIRED)

# ==============================================================================
