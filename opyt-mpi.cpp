//   Copyright 2019 <Huawei Technologies Co., Ltd>
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#include <boost/format.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/operations.hpp>
#include <boost/serialization/complex.hpp>
#include <chrono>
#include <complex>
#include <functional>
#include <iostream>

#include "simulator-mpi/mpi_ext.hpp"
#ifdef _OPENMP
#     include <omp.h>
#endif  // _OPENMP

int main(int argc, char** argv)
{
     boost::mpi::environment env(boost::mpi::threading::level::multiple);
     boost::mpi::communicator world;

     size_t nmax = (1ul << 30);
     int k = 10;
     uint64_t n = (1ul << k);
     if (argc > 1) {
          k = std::atoi(argv[1]);
          n = (1ul << k);
     }

     std::cout << "k: " << k << ", n: " << n << ", world: " << world.size()
               << std::endl;

     size_t n_vectors = 4;
     std::vector<std::vector<std::complex<double>>> svs(n_vectors);
     std::vector<std::vector<std::complex<double>>> rvs(n_vectors);
     for (size_t i = 0; i < n_vectors; ++i) {
          svs[i] = std::vector<std::complex<double>>(n, world.rank());
          rvs[i] = std::vector<std::complex<double>>(n);
     }

     int m = n / world.size();
     std::cout << boost::format("m: %d") % m << std::endl;

     /*
       auto sbuf = sv.data() + m*world.rank();
       auto rbuf = rv.data() + m*world.rank();
       int dest = world.rank() == 0 ? 1 : 0;
       int src = dest;
       MPI_Status status;
     */

     int error = 9999;

     world.barrier();
     auto t0 = std::chrono::high_resolution_clock::now();

     size_t j = 0;
     for (size_t i = 0; i < nmax;) {
          size_t i1 = 0;
          size_t j1 = 0;
#pragma omp parallel for reduction(+ : j1, i1) schedule(static)
          for (size_t k = 0; k < n_vectors; ++k) {
               boost::mpi::all_to_all(world, svs[k], m, rvs[k]);
               i1 += n;
               ++j1;
          }

          j += j1;
          i += i1;
          /*
              error = MPI_Sendrecv(  sbuf, m,
             boost::mpi::get_mpi_datatype<std::complex<double>>(), dest, i ,
             rbuf, m, boost::mpi::get_mpi_datatype<std::complex<double>>(), src,
             i , world, &status);
          */
          //    world.barrier();
     }

     world.barrier();
     auto t1 = std::chrono::high_resolution_clock::now();
     auto dt = std::chrono::duration<double>(t1 - t0).count();

     std::cout << boost::format(
                      "rank: %d, elapsed time: %.3f sec, op time: %.5f ms, "
                      "bandwidth: %.3f GB/s, error: %d")
                      % world.rank() % dt % ((dt * 1000.0) / j)
                      % ((world.size() - 1) * 16.0 * m * j / dt / 1024.0
                         / 1024.0 / 1024.0)
                      % error
               << std::endl;

     return 0;
}
