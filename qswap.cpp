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
#include <chrono>
#include <cstdint>
#include <iostream>

#include "simulator-mpi/funcs.hpp"

using std::cout;
using std::endl;
using namespace boost;

namespace po = boost::program_options;

uint64_t calcSwaps(uint64_t aL, uint64_t aM, std::vector<uint64_t> anInvPerm,
                   std::vector<uint64_t> aPairs)
{
     uint64_t res = 0;

     for (uint64_t rank = 0; rank < (1ul << (aL - aM)); ++rank) {
          auto start = std::chrono::high_resolution_clock::now();

          for (uint64_t index = 0; index < (1ul << aM); ++index) {
               uint64_t new_rank = rank;
               uint64_t new_index = index;
               for (size_t pi = 0; pi < aPairs.size(); pi += 2) {
                    uint64_t remote_qubit = aPairs[pi];
                    uint64_t local_qubit = aPairs[pi + 1];

                    uint64_t remote_bit =
                        (1ul << (anInvPerm[remote_qubit] - aM));
                    uint64_t local_bit = (1ul << anInvPerm[local_qubit]);
                    bool rb = rank & remote_bit;
                    bool ib = index & local_bit;
                    new_index ^= (-rb ^ new_index) & local_bit;
                    new_rank ^= (-ib ^ new_rank) & remote_bit;
               }

               if (new_rank != rank) {
                    res += new_index;
                    //				cout << format("(%d: %d)->(%d: %d)") % rank
                    //% index % new_rank % new_index << endl;
               }
          }

          auto end = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double, std::milli> elapsed_us = end - start;
          std::cout << format("rank: %d, ") % rank
                    << format("elapsed time: %.3f ms") % elapsed_us.count()
                    << endl;
     }

     return res;
}

template <size_t NumPairs>
uint64_t calcSwaps1(uint64_t aL, uint64_t aM, std::vector<uint64_t> aSwapBits)
{
     uint64_t res = 0;

     for (uint64_t rank = 0; rank < (1ul << (aL - aM)); ++rank) {
          auto start = std::chrono::high_resolution_clock::now();

          for (uint64_t index = 0; index < (1ul << aM); ++index) {
               uint64_t new_rank = rank;
               uint64_t new_index = index;

               uint64_t dst_idx = 0;

               for (size_t pi = 0; pi < 2 * NumPairs; pi += 2) {
                    uint64_t remote_bit = aSwapBits[pi];
                    uint64_t local_bit = aSwapBits[pi + 1];

                    bool rb = rank & remote_bit;
                    bool ib = index & local_bit;
                    new_index ^= (-rb ^ new_index) & local_bit;
                    new_rank ^= (-ib ^ new_rank) & remote_bit;

                    dst_idx ^= (-ib ^ dst_idx) & (1ul << (pi / 2));
               }

               res += new_index;

               if (new_rank != rank) {
                    cout << format("(%d: %d)->(%d: %d),  dst_idx: %d") % rank %
                                index % new_rank % new_index % dst_idx
                         << endl;
               }
          }

          auto end = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double, std::milli> elapsed_us = end - start;
          std::cout << format("rank: %d, ") % rank
                    << format("elapsed time: %.3f ms") % elapsed_us.count()
                    << endl;
     }

     return res;
}

template <>
uint64_t calcSwaps1<0>(uint64_t, uint64_t, std::vector<uint64_t>)
{
     return 0;
}

template <size_t NumPairs, class T = uint64_t>
struct CalcSwaps {
     static T doCalc(uint64_t L, uint64_t M, const std::vector<T>& swap_bits)
     {
          if (NumPairs * 2 == swap_bits.size()) {
               return calcSwaps1<NumPairs>(L, M, swap_bits);
          }
          else {
               return CalcSwaps<NumPairs - 1, T>::doCalc(L, M, swap_bits);
          }
     }
};

template <class T>
struct CalcSwaps<0, T> {
     static T doCalc(uint64_t L, uint64_t M, const std::vector<T>& swap_bits)
     {
          return calcSwaps1<0>(L, M, swap_bits);
     }
};

int main(int argc, const char** argv)
{
     uint64_t L = 0;
     uint64_t M = 0;
     std::vector<uint64_t> swap_pairs;
     std::vector<uint64_t> perm_pairs;

     po::options_description desc("Options");
     desc.add_options()("help", "produce help message")(
         "L", po::value<uint64_t>(&L), "total number of qubits")(
         "M", po::value<uint64_t>(&M), "number of local qubits")(
         "swap", po::value<std::vector<uint64_t>>(&swap_pairs)->multitoken(),
         "pair to swap")(
         "perm", po::value<std::vector<uint64_t>>(&perm_pairs)->multitoken(),
         "initial permutation by pairs");

     po::variables_map vm;
     po::store(po::parse_command_line(argc, argv, desc), vm);
     po::notify(vm);

     if (vm.count("help")) {
          cout << desc << "\n";
          return 0;
     }

     cout << format("L: %d, M: %d") % L % M << endl;

     printPairs(cout, "Current permutation:", perm_pairs);
     printPairs(cout, "Do swaps:", swap_pairs);

     auto perm = permuteQubits(naturalOrdering(L), perm_pairs);

     //	auto res = calcSwaps(L, M, inversePermutation(perm), swap_pairs);

     std::vector<uint64_t> swapBits =
         q2bits<uint64_t>(M, swap_pairs, inversePermutation(perm));
     auto res = CalcSwaps<5>::doCalc(L, M, swapBits);

     return res;
}
