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

#pragma once

/*!
 * \file
 * \brief Provide the \c popcnt32 function
 *
 * Either by using processor intrinsics, compiler builtin or by a generic
 * implementation
 */

#ifdef DOXYGEN_DOC
/*!
 * \brief Generic implementation of population count for 32-bit unsigned 
 *        integers
 *
 * Only used if if no-intrinsics or compiler builtin can be found
 *
 * \param a Unsigned integer
 * \return Number of of bit set to \c 1 in integer
 */
uint32_t popcnt32(uint32_t a);
#else
#     ifndef __has_builtin
#          define NO_HAS_BUILTIN
#          define __has_builtin(x) 0
#     endif  // __has_builtin

#     if XSIMD_FALLBACK_X86_INSTR_SET >= XSIMD_FALLBACK_X86_SSE4_2_VERSION
#          include <nmmintrin.h>
#          define popcnt32 _mm_popcnt_u32
#     elif defined(_MSC_VER)
#          include <intrin.h>
#          define popcnt32 __popcnt
#     elif __has_builtin(__builtin_popcount)
#          define popcnt32 __builtin_popcount
#     else
uint32_t popcnt32(uint32_t a)
{
     a -= ((a >> 1) & 0x55555555);
     a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
     return ((a + (a >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}
#     endif  // __has_builtin(__builtin_popcount) || _MSC_VER || >= SSE4

#     ifdef NO_HAS_BUILTIN
#          undef __has_builtin
#     endif  // NO_HAS_BUILTIN
#endif       // !DOXYGEN_DOC
