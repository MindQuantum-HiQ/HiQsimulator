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

#ifndef SWAPARRAYS_HPP
#define SWAPARRAYS_HPP

#include <algorithm>
#include <boost/thread/sync_bounded_queue.hpp>
#include <cstdint>
#include <list>
#include <vector>

template <class T>
struct SwapArrays {
     typedef T value_type;

     std::vector<T> svalues;
     std::vector<T> rvalues;
     std::vector<uint64_t> indices;

     SwapArrays()
     {}

     SwapArrays(SwapArrays&& other)
     {
          std::swap(svalues, other.svalues);
          std::swap(rvalues, other.rvalues);
          std::swap(indices, other.indices);
     }

     size_t size()
     {
          return svalues.size() * sizeof(T);
     }

     void resize(size_t aBytes)
     {
          svalues.resize(aBytes / sizeof(T));
          rvalues.resize(aBytes / sizeof(T));
          indices.resize(aBytes / sizeof(T));
     }

     void reserve(size_t aBytes)
     {
          svalues.reserve(aBytes / sizeof(T));
          rvalues.reserve(aBytes / sizeof(T));
          indices.reserve(aBytes / sizeof(T));
     }
};

template <class T>
struct SwapBuffers {
     typedef SwapArrays<T> swap_arrays_type;

     const size_t maxQueueSize;

     boost::sync_bounded_queue<swap_arrays_type*> fresh_arrays;
     boost::sync_bounded_queue<swap_arrays_type*> fresh_arrays2;
     boost::sync_bounded_queue<swap_arrays_type*> old_arrays;

     explicit SwapBuffers(size_t aMaxQueueSize)
         : maxQueueSize(aMaxQueueSize),
           fresh_arrays(maxQueueSize),
           fresh_arrays2(maxQueueSize),
           old_arrays(maxQueueSize)
     {
          for (size_t i = 0; i < maxQueueSize; ++i) {
               old_arrays.push_back(allocateArrays());
          }
     }

     size_t size()
     {
          return arrs.begin()->size();
     }

     void resize(size_t aSz)
     {
          if (this->size() != aSz) {
               for (auto& arr: arrs)
                    arr.resize(aSz);
          }
     }

private:
     std::list<SwapArrays<T>> arrs;

     swap_arrays_type* allocateArrays()
     {
          arrs.push_back(swap_arrays_type());
          arrs.back().reserve(1ul << 22);
          auto pointer = &arrs.back();
          return pointer;
     }
};

#endif
