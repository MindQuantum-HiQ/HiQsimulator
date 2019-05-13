add_library(permutations STATIC simulator-mpi/funcs.cpp)

target_link_libraries(permutations Boost::boost glog::glog)
