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
#include <boost/program_options.hpp>
#include <cstdint>
#include <iostream>
#include <vector>
#ifdef _OPENMP
#     include <omp.h>
#endif  // _OPENMP
#include <chrono>

using boost::format;
using std::cout;
using std::endl;
using std::vector;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
     uint64_t M = 0;
     uint64_t nthreads = 0;

     po::options_description desc("Options");
     desc.add_options()("help", "produce help message")(
         "M", po::value<uint64_t>(&M), "number of local qubits")(
         "nthreads", po::value<uint64_t>(&nthreads), "number of OMP threads");

     po::variables_map vm;
     po::store(po::parse_command_line(argc, argv, desc), vm);
     po::notify(vm);

     if (vm.count("help")) {
          cout << desc << "\n";
          return 0;
     }

     uint64_t sz = 1ul << M;
     vector<double> v(sz);

     size_t count = 0;
     while (true) {
          auto t0 = std::chrono::high_resolution_clock::now();

#pragma omp parallel for schedule(static)
          for (size_t i = 0; i < sz; ++i) {
               v[i] = v[sz - i - 1] * v[sz - i - 1];
          }

          auto t1 = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double, std::milli> dt_ms = t1 - t0;
          cout << format("count: %d, dt: %.f ms") % count % dt_ms.count()
               << std::endl;
          ++count;
     };

     return 0;
}
