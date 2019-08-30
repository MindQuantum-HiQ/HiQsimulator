add_library(permutations OBJECT simulator-mpi/funcs.cpp)
target_sources(permutations INTERFACE ${SRC_DIR}/simulator-mpi/funcs.hpp)
list(APPEND _doc_targets permutations)
