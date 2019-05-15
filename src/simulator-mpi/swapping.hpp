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

#ifndef SWAPPING_HPP
#define SWAPPING_HPP

#include <glog/logging.h>

#include <boost/format.hpp>
#include <cstdint>
#include <vector>

#include "funcs.hpp"
#include "simulator-mpi/SwapArrays.hpp"

using boost::format;

namespace swapping
{
template <size_t n_bits>
struct SwappingBase {
     template <class T, class Iterator>
     static size_t doCalc(const Iterator state_vector, uint64_t n,
                          size_t start_free_idx, T* svalues, uint64_t* indices,
                          const std::vector<uint64_t>& swap_bits)
     {
          size_t free_idx_end = start_free_idx + n;
          for (size_t free_idx = start_free_idx; free_idx < free_idx_end;
               ++free_idx) {
               size_t res_free_idx = free_idx;
               for (size_t bit_idx = 0; bit_idx < n_bits; ++bit_idx) {
                    uint64_t sw_bit = swap_bits[bit_idx];
                    uint64_t l = res_free_idx & (sw_bit - 1);
                    uint64_t r = res_free_idx - l;

                    res_free_idx = 2 * r + l;
               }

               for (size_t swap_idx = 0; swap_idx < (1ul << n_bits);
                    ++swap_idx) {
                    size_t res_idx = res_free_idx;
                    size_t res_swap_idx = 0;
                    for (size_t bi = 0; bi < n_bits; ++bi) {
                         bool bv = swap_idx & (1ul << bi);
                         res_swap_idx |=
                             bv * (swap_bits[swap_bits[n_bits + bi]]);
                    }

                    res_idx += res_swap_idx;
                    size_t idx = n * swap_idx + free_idx - start_free_idx;
                    indices[idx] = res_idx;
                    svalues[idx] = state_vector[res_idx];
               }
          }

          return free_idx_end;
     }
};

template <size_t n_bits_max>
struct Swapping {
     template <class T, class Iterator>
     static size_t calcSwap(uint64_t n_bits, const Iterator state_vector,
                            uint64_t n, size_t start_free_idx, T* svalues,
                            uint64_t* indices,
                            const std::vector<uint64_t>& swap_bits)
     {
          if (n_bits == n_bits_max) {
               return SwappingBase<n_bits_max>::doCalc(state_vector, n,
                                                       start_free_idx, svalues,
                                                       indices, swap_bits);
          }
          else {
               return Swapping<n_bits_max - 1>::calcSwap(
                   n_bits, state_vector, n, start_free_idx, svalues, indices,
                   swap_bits);
          }
     }
};

template <>
struct Swapping<0> {
     template <class T, class Iterator>
     static size_t calcSwap(uint64_t, const Iterator, uint64_t, size_t, T*,
                            uint64_t*, const std::vector<uint64_t>&)
     {
          return 0;
     }
};

}  // namespace swapping

#endif  // SWAPPING_HPP
