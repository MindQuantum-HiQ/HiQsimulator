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

#ifdef _OPENMP
#     include <omp.h>
#endif  // _OPENMP
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "simulator-mpi/funcs.hpp"
#include "simulator-mpi/fusion_mpi.hpp"
#include "simulator-mpi/kernels/intrin/kernels.hpp"
#include "simulator-mpi/kernels/nointrin/kernels.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::size_t;
using namespace boost;
using boost::format;

namespace bc = boost::container;
namespace po = boost::program_options;

typedef std::vector<std::complex<double>> StateVector;

template <class V, class Mat>
inline void kernel_core_print_idx(V& psi, std::size_t I, std::size_t d0,
                                  Mat const& m)
{
     //     std::complex<double> v[2];
     //     v[0] = psi[I];
     //     v[1] = psi[I + d0];
     //
     //     psi[I] = (add(mul(v[0], m[0][0]), mul(v[1], m[0][1])));
     //     psi[I + d0] = (add(mul(v[0], m[1][0]), mul(v[1], m[1][1])));
     uint64_t M = log2(psi.size());
     cout << format("i0: %d") << bitstring(I, M) << endl;
     cout << format("i1: %d") << bitstring(I + d0, M) << endl;
}

template <class V, class Mat>
inline void kernel_core_print_idx(V& psi, std::size_t I, std::size_t d0,
                                  std::size_t d1, Mat const& m)
{
     //     std::complex<double> v[4];
     //     v[0] = psi[I];
     //     v[1] = psi[I + d0];
     //     v[2] = psi[I + d1];
     //     v[3] = psi[I + d0 + d1];
     //
     //     psi[I] = (add(mul(v[0], m[0][0]), add(mul(v[1], m[0][1]),
     //     add(mul(v[2], m[0][2]), mul(v[3], m[0][3]))))); psi[I + d0] =
     //     (add(mul(v[0], m[1][0]), add(mul(v[1], m[1][1]), add(mul(v[2],
     //     m[1][2]), mul(v[3], m[1][3]))))); psi[I + d1] = (add(mul(v[0],
     //     m[2][0]), add(mul(v[1], m[2][1]), add(mul(v[2], m[2][2]), mul(v[3],
     //     m[2][3]))))); psi[I + d0 + d1] = (add(mul(v[0], m[3][0]),
     //     add(mul(v[1], m[3][1]), add(mul(v[2], m[3][2]), mul(v[3],
     //     m[3][3])))));

     uint64_t M = log2(psi.size());
     cout << format("i0: %d") % bitstring(I, M) << endl;
     cout << format("i1: %d") % bitstring(I + d0, M) << endl;
     cout << format("i2: %d") % bitstring(I + d1, M) << endl;
     cout << format("i3: %d") % bitstring(I + d0 + d1, M) << endl;
}

template <class V, class Mat>
inline void kernel_core_set_psi(V& psi, std::size_t I, std::size_t d0,
                                std::size_t d1, Mat const& m)
{
     //     std::complex<double> v[4];
     //     v[0] = psi[I];
     //     v[1] = psi[I + d0];
     //     v[2] = psi[I + d1];
     //     v[3] = psi[I + d0 + d1];
     //
     psi[I] = I;
     psi[I + d0] = I + d0;
     psi[I + d1] = I + d1;
     psi[I + d0 + d1] = I + d0 + d1;
}

uint64_t get_control_mask(const std::vector<uint64_t>& ctrls)
{
     uint64_t res = 0;

     for (auto c: ctrls)
          res |= (1ul << c);

     return res;
}

template <size_t n_bits, class V, class Mat,
          void K(V&, std::size_t, std::size_t, std::size_t, Mat const&)>
void iterate_state_vector(uint64_t M, V& vec, const Mat& m,
                          const std::vector<uint64_t>& trgts,
                          const std::vector<std::pair<uint64_t, bool>>& bits)
{
     for (size_t free_idx = 0; free_idx < (1ul << (M - n_bits)); ++free_idx) {
          size_t res_start_idx = free_idx;
          for (size_t bit_idx = 0; bit_idx < n_bits; ++bit_idx) {
               uint64_t b = (1ul << bits[bit_idx].first);
               uint64_t l = res_start_idx & (b - 1);
               uint64_t r = res_start_idx - l;

               res_start_idx = 2 * r + l;
               //             cout << format("bit: %2d, res_start_idx: %s") %
               //             bits[bit_idx].first % bitstring(res_start_idx, M)
               //             << endl;
               res_start_idx |= bits[bit_idx].second * b;
               //             cout << format("         res_start_idx: %s") %
               //             bitstring(res_start_idx, M) << endl;
          }

          //         cout << format("res_start_idx: %s") % bitstring(
          //         res_start_idx, M) << endl;

          K(vec, res_start_idx, 1ul << trgts[0], 1ul << trgts[1], m);
     }
}

Fusion::Matrix construct_gate(size_t n)
{
     Fusion::Matrix m;

     size_t N = 1ul << n;

     m.resize(N);
     for (auto& r: m) {
          r.resize(N);
     }

     return m;
}

void apply_gate(StateVector& vec_, const Fusion::Matrix& m,
                const std::vector<uint64_t>& trgts, uint64_t ctrlmask)
{
     auto t0 = std::chrono::high_resolution_clock::now();

     switch (trgts.size()) {
          case 1:
#pragma omp parallel
               intrin::kernelK<StateVector, Fusion::Matrix,
                               intrin::kernel_core>(vec_, trgts[0], m,
                                                    ctrlmask);
               break;
          case 2:
#pragma omp parallel
               intrin::kernelK<StateVector, Fusion::Matrix,
                               intrin::kernel_core>(vec_, trgts[1], trgts[0], m,
                                                    ctrlmask);
               break;
          case 3:
#pragma omp parallel
               intrin::kernelK<StateVector, Fusion::Matrix,
                               intrin::kernel_core>(vec_, trgts[2], trgts[1],
                                                    trgts[0], m, ctrlmask);
               break;
          case 4:
#pragma omp parallel
               intrin::kernelK<StateVector, Fusion::Matrix,
                               intrin::kernel_core>(
                   vec_, trgts[3], trgts[2], trgts[1], trgts[0], m, ctrlmask);
               break;
          case 5:
#pragma omp parallel
               intrin::kernelK<StateVector, Fusion::Matrix,
                               intrin::kernel_core>(vec_, trgts[4], trgts[3],
                                                    trgts[2], trgts[1],
                                                    trgts[0], m, ctrlmask);
               break;
     }

     auto t1 = std::chrono::high_resolution_clock::now();
     std::chrono::duration<double, std::milli> dt = t1 - t0;
     cout << format("trgts: %d dt: %.3f ms") % trgts.size() % dt.count()
          << endl;
}

void hiqubit(uint64_t M)
{
     StateVector vec_(1ul << M);

     for (uint64_t n: {1, 2, 3, 4, 5}) {
          std::vector<uint64_t> trgts(n);
          Fusion::Matrix m = construct_gate(trgts.size());

          for (uint64_t q = 0; q < M - n + 1; ++q) {
               for (size_t i = 0; i < n; ++i) {
                    trgts[i] = q + i;
               }
               //              std::cout << std::endl << "trgs: " <<
               //              print(trgts) << std::endl;

               apply_gate(vec_, m, trgts, 0ul);
          }
     }
}

void orig_algo(uint64_t L, uint64_t M, std::vector<uint64_t>& trgts,
               std::vector<uint64_t>& ctrls)
{
     StateVector vec_(1ul << M);
     Fusion::Matrix m = construct_gate(trgts.size());

     std::sort(trgts.begin(), trgts.end());
     std::sort(ctrls.begin(), ctrls.end());

     uint64_t ctrlmask = get_control_mask(ctrls);

     //     cout << format("ctrlmask: %s") % bitstring(ctrlmask, M) << endl;

     apply_gate(vec_, m, trgts, ctrlmask);
}

int new_algo(uint64_t L, uint64_t M, std::vector<uint64_t>& trgts,
             std::vector<uint64_t>& ctrls)
{
     StateVector vec(1ul << M);
     Fusion::Matrix m;

     std::vector<std::pair<uint64_t, bool>> bits(ctrls.size() + trgts.size());
     for (size_t i = 0; i < ctrls.size(); ++i) {
          bits[i] = std::make_pair(ctrls[i], true);
     }

     for (size_t i = ctrls.size(); i < bits.size(); ++i) {
          bits[i] = std::make_pair(trgts[i - ctrls.size()], false);
     }

     std::sort(bits.begin(), bits.end(),
               [](std::pair<uint64_t, bool>& a, std::pair<uint64_t, bool>& b)
                   -> bool { return a.first < b.first; });

     //     for(auto& p: bits) {
     //         cout << format("%d: %d") % p.first % p.second << endl;
     //     }
     auto t0 = std::chrono::high_resolution_clock::now();

     switch (trgts.size()) {
          case 2:
               iterate_state_vector<2, StateVector, Fusion::Matrix,
                                    kernel_core_set_psi>(M, vec, m, trgts,
                                                         bits);
               break;
     }

     auto t1 = std::chrono::high_resolution_clock::now();
     std::chrono::duration<double, std::milli> dt = t1 - t0;
     cout << format("dt: %.3f ms") % dt.count() << endl;

     //     size_t n_bits = bits.size();
     //     for(size_t free_idx = 0; free_idx < (1ul << (M - n_bits));
     //     ++free_idx) {
     //         size_t res_start_idx = free_idx;
     //         for(size_t bit_idx = 0; bit_idx < n_bits; ++bit_idx ) {
     //             uint64_t b = (1ul << bits[bit_idx].first);
     //             uint64_t l = res_start_idx & (b - 1);
     //             uint64_t r = res_start_idx - l;
     //
     //             res_start_idx = 2*r + l;
     // //             cout << format("bit: %2d, res_start_idx: %s") %
     // bits[bit_idx].first % bitstring(res_start_idx, M) << endl;
     //             res_start_idx |= bits[bit_idx].second*b;
     // //             cout << format("         res_start_idx: %s") %
     // bitstring(res_start_idx, M) << endl;
     //         }
     //
     // //         cout << format("res_start_idx: %s") % bitstring(
     // res_start_idx, M) << endl;
     //
     //         switch(trgts.size()) {
     //             case 1:
     // //                 cout << format("i0: %s") % bitstring( res_start_idx,
     // M) << endl;
     // //                 cout << format("i1: %s") % bitstring( res_start_idx +
     // (1ul << trgts[0]), M) << endl;
     //                 kernel_core_print_idx(vec, res_start_idx, 1ul <<
     //                 trgts[0], m); break;
     //             case 2:
     //                 kernel_core_print_idx(vec, res_start_idx, 1ul <<
     //                 trgts[0], 1ul << trgts[1], m);
     // //                 cout << format("i0: %s") % bitstring( res_start_idx,
     // M) << endl;
     // //                 cout << format("i1: %s") % bitstring( res_start_idx +
     // (1ul << trgts[0]), M) << endl;
     // //                 cout << format("i2: %s") % bitstring( res_start_idx +
     // (1ul << trgts[1]), M) << endl;
     // //                 cout << format("i3: %s") % bitstring( res_start_idx +
     // (1ul << trgts[0]) + (1ul << trgts[1]), M) << endl;
     //                 break;
     //         }
     //     }

     return 0;
}

int main(int argc, char** argv)
{
     uint64_t L = 0;
     uint64_t M = 0;
#ifdef _OPENMP
     uint64_t nthreads = 1;
#endif  // _OPENMP
     uint64_t rank = 0;
     std::vector<uint64_t> trgts;
     std::vector<uint64_t> ctrls;

     std::string algo;

     po::options_description desc("Options");
     desc.add_options()("help", "produce help message")(
         "L", po::value<uint64_t>(&L), "total number of qubits")(
         "M", po::value<uint64_t>(&M), "number of local qubits")
#ifdef _OPENMP
         ("nthreads", po::value<uint64_t>(&nthreads), "number of OMP threads")
#endif  // _OPENMP
             ("rank", po::value<uint64_t>(&rank), "rank")(
                 "trgts",
                 po::value<std::vector<uint64_t>>(&trgts)->multitoken(),
                 "target qubits")(
                 "ctrls",
                 po::value<std::vector<uint64_t>>(&ctrls)->multitoken(),
                 "control qubits")(
                 "algo", po::value<std::string>(&algo)->required(), "algoritm");

     po::variables_map vm;
     po::store(po::parse_command_line(argc, argv, desc), vm);
     po::notify(vm);

     if (vm.count("help")) {
          cout << desc << "\n";
          return 0;
     }

     cout << format("algo: %s, L: %d, M: %d, trgts: %s, ctrls: %s") % algo % L %
                 M % print(trgts) % print(ctrls)
          << endl;
#ifdef _OPENMP
     omp_set_num_threads(nthreads);
#endif  // _OPENMP

     if (algo == "orig") {
          orig_algo(L, M, trgts, ctrls);
     }
     else if (algo == "new") {
          new_algo(L, M, trgts, ctrls);
     }
     else if (algo == "hiqubit") {
          hiqubit(M);
     }
     else {
          cout << "ERROR: unknown --algo " << algo << endl;
     }

     return 0;
}
