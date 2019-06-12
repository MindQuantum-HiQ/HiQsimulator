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

#ifndef SCHEDULER_DEFS_H
#define SCHEDULER_DEFS_H

#include <cstdint>
#ifdef _MSC_VER
#     include <intrin.h>
#     define __builtin_popcountll __popcnt64
#endif  // _MSC_VER

typedef uint64_t msk_t;
typedef int64_t id_num_t;

#define set_bit(x, y) (x |= static_cast<msk_t>(1) << static_cast<msk_t>(y))
#define test_bit(x, y) ((x >> y) & 1UL)
#define count_bits(x) (__builtin_popcountll(static_cast<uint64_t>(x)))
#define inter(x, y) (x & y)
#define unite(x, y) (x | y)
#define submask(x, y) (!(x & ~y))

#endif  // SCHEDULER_DEFS_H
