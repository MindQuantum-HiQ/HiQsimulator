add_library(permutations OBJECT simulator-mpi/funcs.cpp)

target_link_libraries(permutations Boost::boost glog::glog)
