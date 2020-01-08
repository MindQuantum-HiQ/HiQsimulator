define_object_library(permutations
                      SOURCES
                      ${CMAKE_CURRENT_LIST_DIR}/simulator-mpi/funcs.cpp
                      HEADERS
                      ${CMAKE_CURRENT_LIST_DIR}/simulator-mpi/funcs.hpp
                      DEPENDENCIES
                      glog::glog
                      Boost::boost)
