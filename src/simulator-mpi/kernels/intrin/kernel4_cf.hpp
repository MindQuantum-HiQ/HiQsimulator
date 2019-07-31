// Copyright 2017 ProjectQ-Framework (www.projectq.ch)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INTRIN_CF_BUFFER
#     error INTRIN_CF_BUFFER must be defined!
#endif  // !INTRIN_CF_BUFFER

#define buf_sz INTRIN_CF_BUFFER

namespace intrin_cf
{
using intrin::add;
using intrin::load;
using intrin::load2;
using intrin::mul;
using intrin::store;

auto get_idx(std::size_t n, std::size_t m0, std::size_t m1, std::size_t m2,
             std::size_t m3, std::size_t m4)
{
     return (n & m0) ^ ((n & m1) << 1) ^ ((n & m2) << 2) ^ ((n & m3) << 3)
            ^ ((n & m4) << 4);
}

template <class V>
inline void kernel_compute(V &psi, std::size_t I, std::size_t d0,
                           std::size_t d1, std::size_t d2, std::size_t d3,
                           const __m256d m[], const __m256d mt[],
                           std::size_t m0, std::size_t m1, std::size_t m2,
                           std::size_t m3, std::size_t m4)
{
     __m256d v;
     __m256d tmp[buf_sz * 8];
     size_t idxes[buf_sz];
     for (size_t i = 0; i < buf_sz; ++i) {
          idxes[i] = get_idx(I + i, m0, m1, m2, m3, m4);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i]]);
          tmp[i] = mul(v, m[0], mt[0]);
          tmp[buf_sz + i] = mul(v, m[4], mt[4]);
          tmp[2 * buf_sz + i] = mul(v, m[8], mt[8]);
          tmp[3 * buf_sz + i] = mul(v, m[12], mt[12]);
          tmp[4 * buf_sz + i] = mul(v, m[16], mt[16]);
          tmp[5 * buf_sz + i] = mul(v, m[20], mt[20]);
          tmp[6 * buf_sz + i] = mul(v, m[24], mt[24]);
          tmp[7 * buf_sz + i] = mul(v, m[28], mt[28]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d0]);
          tmp[i] = add(tmp[i], mul(v, m[1], mt[1]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[5], mt[5]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[9], mt[9]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[13], mt[13]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[17], mt[17]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[21], mt[21]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[25], mt[25]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[29], mt[29]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d1]);
          tmp[i] = add(tmp[i], mul(v, m[2], mt[2]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[6], mt[6]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[10], mt[10]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[14], mt[14]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[18], mt[18]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[22], mt[22]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[26], mt[26]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[30], mt[30]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d1 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[3], mt[3]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[7], mt[7]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[11], mt[11]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[15], mt[15]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[19], mt[19]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[23], mt[23]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[27], mt[27]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[31], mt[31]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d2]);
          tmp[i] = add(tmp[i], mul(v, m[32], mt[32]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[36], mt[36]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[40], mt[40]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[44], mt[44]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[48], mt[48]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[52], mt[52]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[56], mt[56]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[60], mt[60]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d2 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[33], mt[33]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[37], mt[37]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[41], mt[41]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[45], mt[45]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[49], mt[49]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[53], mt[53]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[57], mt[57]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[61], mt[61]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d2 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[34], mt[34]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[38], mt[38]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[42], mt[42]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[46], mt[46]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[50], mt[50]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[54], mt[54]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[58], mt[58]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[62], mt[62]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d2 + d0 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[35], mt[35]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[39], mt[39]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[43], mt[43]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[47], mt[47]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[51], mt[51]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[55], mt[55]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[59], mt[59]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[63], mt[63]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3]);
          tmp[i] = add(tmp[i], mul(v, m[64], mt[64]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[68], mt[68]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[72], mt[72]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[76], mt[76]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[80], mt[80]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[84], mt[84]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[88], mt[88]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[92], mt[92]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[65], mt[65]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[69], mt[69]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[73], mt[73]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[77], mt[77]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[81], mt[81]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[85], mt[85]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[89], mt[89]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[93], mt[93]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[66], mt[66]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[70], mt[70]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[74], mt[74]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[78], mt[78]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[82], mt[82]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[86], mt[86]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[90], mt[90]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[94], mt[94]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d0 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[67], mt[67]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[71], mt[71]));
          tmp[2 * buf_sz + i] = add(tmp[2 * buf_sz + i], mul(v, m[75], mt[75]));
          tmp[3 * buf_sz + i] = add(tmp[3 * buf_sz + i], mul(v, m[79], mt[79]));
          tmp[4 * buf_sz + i] = add(tmp[4 * buf_sz + i], mul(v, m[83], mt[83]));
          tmp[5 * buf_sz + i] = add(tmp[5 * buf_sz + i], mul(v, m[87], mt[87]));
          tmp[6 * buf_sz + i] = add(tmp[6 * buf_sz + i], mul(v, m[91], mt[91]));
          tmp[7 * buf_sz + i] = add(tmp[7 * buf_sz + i], mul(v, m[95], mt[95]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2]);
          tmp[i] = add(tmp[i], mul(v, m[96], mt[96]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[100], mt[100]));
          tmp[2 * buf_sz + i]
              = add(tmp[2 * buf_sz + i], mul(v, m[104], mt[104]));
          tmp[3 * buf_sz + i]
              = add(tmp[3 * buf_sz + i], mul(v, m[108], mt[108]));
          tmp[4 * buf_sz + i]
              = add(tmp[4 * buf_sz + i], mul(v, m[112], mt[112]));
          tmp[5 * buf_sz + i]
              = add(tmp[5 * buf_sz + i], mul(v, m[116], mt[116]));
          tmp[6 * buf_sz + i]
              = add(tmp[6 * buf_sz + i], mul(v, m[120], mt[120]));
          tmp[7 * buf_sz + i]
              = add(tmp[7 * buf_sz + i], mul(v, m[124], mt[124]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[97], mt[97]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[101], mt[101]));
          tmp[2 * buf_sz + i]
              = add(tmp[2 * buf_sz + i], mul(v, m[105], mt[105]));
          tmp[3 * buf_sz + i]
              = add(tmp[3 * buf_sz + i], mul(v, m[109], mt[109]));
          tmp[4 * buf_sz + i]
              = add(tmp[4 * buf_sz + i], mul(v, m[113], mt[113]));
          tmp[5 * buf_sz + i]
              = add(tmp[5 * buf_sz + i], mul(v, m[117], mt[117]));
          tmp[6 * buf_sz + i]
              = add(tmp[6 * buf_sz + i], mul(v, m[121], mt[121]));
          tmp[7 * buf_sz + i]
              = add(tmp[7 * buf_sz + i], mul(v, m[125], mt[125]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[98], mt[98]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[102], mt[102]));
          tmp[2 * buf_sz + i]
              = add(tmp[2 * buf_sz + i], mul(v, m[106], mt[106]));
          tmp[3 * buf_sz + i]
              = add(tmp[3 * buf_sz + i], mul(v, m[110], mt[110]));
          tmp[4 * buf_sz + i]
              = add(tmp[4 * buf_sz + i], mul(v, m[114], mt[114]));
          tmp[5 * buf_sz + i]
              = add(tmp[5 * buf_sz + i], mul(v, m[118], mt[118]));
          tmp[6 * buf_sz + i]
              = add(tmp[6 * buf_sz + i], mul(v, m[122], mt[122]));
          tmp[7 * buf_sz + i]
              = add(tmp[7 * buf_sz + i], mul(v, m[126], mt[126]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2 + d1 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[99], mt[99]));
          tmp[buf_sz + i] = add(tmp[buf_sz + i], mul(v, m[103], mt[103]));
          tmp[2 * buf_sz + i]
              = add(tmp[2 * buf_sz + i], mul(v, m[107], mt[107]));
          tmp[3 * buf_sz + i]
              = add(tmp[3 * buf_sz + i], mul(v, m[111], mt[111]));
          tmp[4 * buf_sz + i]
              = add(tmp[4 * buf_sz + i], mul(v, m[115], mt[115]));
          tmp[5 * buf_sz + i]
              = add(tmp[5 * buf_sz + i], mul(v, m[119], mt[119]));
          tmp[6 * buf_sz + i]
              = add(tmp[6 * buf_sz + i], mul(v, m[123], mt[123]));
          tmp[7 * buf_sz + i]
              = add(tmp[7 * buf_sz + i], mul(v, m[127], mt[127]));
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i]], &psi[idxes[i] + d0], tmp[i]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i] + d1], &psi[idxes[i] + d1 + d0], tmp[buf_sz + i]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i] + d2], &psi[idxes[i] + d2 + d0],
                tmp[2 * buf_sz + i]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i] + d2 + d1], &psi[idxes[i] + d2 + d1 + d0],
                tmp[3 * buf_sz + i]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i] + d3], &psi[idxes[i] + d3 + d0],
                tmp[4 * buf_sz + i]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i] + d3 + d1], &psi[idxes[i] + d3 + d1 + d0],
                tmp[5 * buf_sz + i]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i] + d3 + d2], &psi[idxes[i] + d3 + d2 + d0],
                tmp[6 * buf_sz + i]);
     }
     for (unsigned i = 0; i < buf_sz; ++i) {
          store(&psi[idxes[i] + d3 + d2 + d1],
                &psi[idxes[i] + d3 + d2 + d1 + d0], tmp[7 * buf_sz + i]);
     }
}

template <class V>
inline void kernel_compute(V &psi, std::size_t I, std::size_t d0,
                           std::size_t d1, std::size_t d2, std::size_t d3,
                           const __m256d m[], const __m256d mt[],
                           std::size_t m0, std::size_t m1, std::size_t m2,
                           std::size_t m3, std::size_t m4, std::size_t cmask)
{
     __m256d v;
     __m256d tmp[buf_sz * 8];
     size_t idxes[buf_sz];
     std::size_t real_sz = 0;
     for (size_t i = 0; i < buf_sz; ++i) {
          size_t tmp;
          tmp = get_idx(I + i, m0, m1, m2, m3, m4);
          if ((tmp & cmask) == cmask) {
               idxes[real_sz] = tmp;
               ++real_sz;
          }
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i]]);
          tmp[i] = mul(v, m[0], mt[0]);
          tmp[real_sz + i] = mul(v, m[4], mt[4]);
          tmp[2 * real_sz + i] = mul(v, m[8], mt[8]);
          tmp[3 * real_sz + i] = mul(v, m[12], mt[12]);
          tmp[4 * real_sz + i] = mul(v, m[16], mt[16]);
          tmp[5 * real_sz + i] = mul(v, m[20], mt[20]);
          tmp[6 * real_sz + i] = mul(v, m[24], mt[24]);
          tmp[7 * real_sz + i] = mul(v, m[28], mt[28]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d0]);
          tmp[i] = add(tmp[i], mul(v, m[1], mt[1]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[5], mt[5]));
          tmp[2 * real_sz + i] = add(tmp[2 * real_sz + i], mul(v, m[9], mt[9]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[13], mt[13]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[17], mt[17]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[21], mt[21]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[25], mt[25]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[29], mt[29]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d1]);
          tmp[i] = add(tmp[i], mul(v, m[2], mt[2]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[6], mt[6]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[10], mt[10]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[14], mt[14]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[18], mt[18]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[22], mt[22]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[26], mt[26]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[30], mt[30]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d1 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[3], mt[3]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[7], mt[7]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[11], mt[11]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[15], mt[15]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[19], mt[19]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[23], mt[23]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[27], mt[27]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[31], mt[31]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d2]);
          tmp[i] = add(tmp[i], mul(v, m[32], mt[32]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[36], mt[36]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[40], mt[40]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[44], mt[44]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[48], mt[48]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[52], mt[52]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[56], mt[56]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[60], mt[60]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d2 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[33], mt[33]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[37], mt[37]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[41], mt[41]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[45], mt[45]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[49], mt[49]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[53], mt[53]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[57], mt[57]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[61], mt[61]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d2 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[34], mt[34]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[38], mt[38]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[42], mt[42]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[46], mt[46]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[50], mt[50]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[54], mt[54]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[58], mt[58]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[62], mt[62]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d2 + d0 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[35], mt[35]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[39], mt[39]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[43], mt[43]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[47], mt[47]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[51], mt[51]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[55], mt[55]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[59], mt[59]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[63], mt[63]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3]);
          tmp[i] = add(tmp[i], mul(v, m[64], mt[64]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[68], mt[68]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[72], mt[72]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[76], mt[76]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[80], mt[80]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[84], mt[84]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[88], mt[88]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[92], mt[92]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[65], mt[65]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[69], mt[69]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[73], mt[73]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[77], mt[77]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[81], mt[81]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[85], mt[85]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[89], mt[89]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[93], mt[93]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[66], mt[66]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[70], mt[70]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[74], mt[74]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[78], mt[78]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[82], mt[82]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[86], mt[86]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[90], mt[90]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[94], mt[94]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d0 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[67], mt[67]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[71], mt[71]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[75], mt[75]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[79], mt[79]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[83], mt[83]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[87], mt[87]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[91], mt[91]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[95], mt[95]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2]);
          tmp[i] = add(tmp[i], mul(v, m[96], mt[96]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[100], mt[100]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[104], mt[104]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[108], mt[108]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[112], mt[112]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[116], mt[116]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[120], mt[120]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[124], mt[124]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[97], mt[97]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[101], mt[101]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[105], mt[105]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[109], mt[109]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[113], mt[113]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[117], mt[117]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[121], mt[121]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[125], mt[125]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2 + d1]);
          tmp[i] = add(tmp[i], mul(v, m[98], mt[98]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[102], mt[102]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[106], mt[106]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[110], mt[110]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[114], mt[114]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[118], mt[118]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[122], mt[122]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[126], mt[126]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          v = load2(&psi[idxes[i] + d3 + d2 + d1 + d0]);
          tmp[i] = add(tmp[i], mul(v, m[99], mt[99]));
          tmp[real_sz + i] = add(tmp[real_sz + i], mul(v, m[103], mt[103]));
          tmp[2 * real_sz + i]
              = add(tmp[2 * real_sz + i], mul(v, m[107], mt[107]));
          tmp[3 * real_sz + i]
              = add(tmp[3 * real_sz + i], mul(v, m[111], mt[111]));
          tmp[4 * real_sz + i]
              = add(tmp[4 * real_sz + i], mul(v, m[115], mt[115]));
          tmp[5 * real_sz + i]
              = add(tmp[5 * real_sz + i], mul(v, m[119], mt[119]));
          tmp[6 * real_sz + i]
              = add(tmp[6 * real_sz + i], mul(v, m[123], mt[123]));
          tmp[7 * real_sz + i]
              = add(tmp[7 * real_sz + i], mul(v, m[127], mt[127]));
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i]], &psi[idxes[i] + d0], tmp[i]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i] + d1], &psi[idxes[i] + d1 + d0],
                tmp[real_sz + i]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i] + d2], &psi[idxes[i] + d2 + d0],
                tmp[2 * real_sz + i]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i] + d2 + d1], &psi[idxes[i] + d2 + d1 + d0],
                tmp[3 * real_sz + i]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i] + d3], &psi[idxes[i] + d3 + d0],
                tmp[4 * real_sz + i]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i] + d3 + d1], &psi[idxes[i] + d3 + d1 + d0],
                tmp[5 * real_sz + i]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i] + d3 + d2], &psi[idxes[i] + d3 + d2 + d0],
                tmp[6 * real_sz + i]);
     }
     for (unsigned i = 0; i < real_sz; ++i) {
          store(&psi[idxes[i] + d3 + d2 + d1],
                &psi[idxes[i] + d3 + d2 + d1 + d0], tmp[7 * real_sz + i]);
     }
}

template <class V, class M>
inline void kernel_core(V &v, unsigned id3, unsigned id2, unsigned id1,
                        unsigned id0, M const &m, std::size_t ctrlmask)
{
     const auto n = v.size();
     const std::size_t d0 = 1UL << id0;
     const std::size_t d1 = 1UL << id1;
     const std::size_t d2 = 1UL << id2;
     const std::size_t d3 = 1UL << id3;
     const __m256d mm[]
         = {load(&m[0][0], &m[1][0]),     load(&m[0][1], &m[1][1]),
            load(&m[0][2], &m[1][2]),     load(&m[0][3], &m[1][3]),
            load(&m[2][0], &m[3][0]),     load(&m[2][1], &m[3][1]),
            load(&m[2][2], &m[3][2]),     load(&m[2][3], &m[3][3]),
            load(&m[4][0], &m[5][0]),     load(&m[4][1], &m[5][1]),
            load(&m[4][2], &m[5][2]),     load(&m[4][3], &m[5][3]),
            load(&m[6][0], &m[7][0]),     load(&m[6][1], &m[7][1]),
            load(&m[6][2], &m[7][2]),     load(&m[6][3], &m[7][3]),
            load(&m[8][0], &m[9][0]),     load(&m[8][1], &m[9][1]),
            load(&m[8][2], &m[9][2]),     load(&m[8][3], &m[9][3]),
            load(&m[10][0], &m[11][0]),   load(&m[10][1], &m[11][1]),
            load(&m[10][2], &m[11][2]),   load(&m[10][3], &m[11][3]),
            load(&m[12][0], &m[13][0]),   load(&m[12][1], &m[13][1]),
            load(&m[12][2], &m[13][2]),   load(&m[12][3], &m[13][3]),
            load(&m[14][0], &m[15][0]),   load(&m[14][1], &m[15][1]),
            load(&m[14][2], &m[15][2]),   load(&m[14][3], &m[15][3]),
            load(&m[0][4], &m[1][4]),     load(&m[0][5], &m[1][5]),
            load(&m[0][6], &m[1][6]),     load(&m[0][7], &m[1][7]),
            load(&m[2][4], &m[3][4]),     load(&m[2][5], &m[3][5]),
            load(&m[2][6], &m[3][6]),     load(&m[2][7], &m[3][7]),
            load(&m[4][4], &m[5][4]),     load(&m[4][5], &m[5][5]),
            load(&m[4][6], &m[5][6]),     load(&m[4][7], &m[5][7]),
            load(&m[6][4], &m[7][4]),     load(&m[6][5], &m[7][5]),
            load(&m[6][6], &m[7][6]),     load(&m[6][7], &m[7][7]),
            load(&m[8][4], &m[9][4]),     load(&m[8][5], &m[9][5]),
            load(&m[8][6], &m[9][6]),     load(&m[8][7], &m[9][7]),
            load(&m[10][4], &m[11][4]),   load(&m[10][5], &m[11][5]),
            load(&m[10][6], &m[11][6]),   load(&m[10][7], &m[11][7]),
            load(&m[12][4], &m[13][4]),   load(&m[12][5], &m[13][5]),
            load(&m[12][6], &m[13][6]),   load(&m[12][7], &m[13][7]),
            load(&m[14][4], &m[15][4]),   load(&m[14][5], &m[15][5]),
            load(&m[14][6], &m[15][6]),   load(&m[14][7], &m[15][7]),
            load(&m[0][8], &m[1][8]),     load(&m[0][9], &m[1][9]),
            load(&m[0][10], &m[1][10]),   load(&m[0][11], &m[1][11]),
            load(&m[2][8], &m[3][8]),     load(&m[2][9], &m[3][9]),
            load(&m[2][10], &m[3][10]),   load(&m[2][11], &m[3][11]),
            load(&m[4][8], &m[5][8]),     load(&m[4][9], &m[5][9]),
            load(&m[4][10], &m[5][10]),   load(&m[4][11], &m[5][11]),
            load(&m[6][8], &m[7][8]),     load(&m[6][9], &m[7][9]),
            load(&m[6][10], &m[7][10]),   load(&m[6][11], &m[7][11]),
            load(&m[8][8], &m[9][8]),     load(&m[8][9], &m[9][9]),
            load(&m[8][10], &m[9][10]),   load(&m[8][11], &m[9][11]),
            load(&m[10][8], &m[11][8]),   load(&m[10][9], &m[11][9]),
            load(&m[10][10], &m[11][10]), load(&m[10][11], &m[11][11]),
            load(&m[12][8], &m[13][8]),   load(&m[12][9], &m[13][9]),
            load(&m[12][10], &m[13][10]), load(&m[12][11], &m[13][11]),
            load(&m[14][8], &m[15][8]),   load(&m[14][9], &m[15][9]),
            load(&m[14][10], &m[15][10]), load(&m[14][11], &m[15][11]),
            load(&m[0][12], &m[1][12]),   load(&m[0][13], &m[1][13]),
            load(&m[0][14], &m[1][14]),   load(&m[0][15], &m[1][15]),
            load(&m[2][12], &m[3][12]),   load(&m[2][13], &m[3][13]),
            load(&m[2][14], &m[3][14]),   load(&m[2][15], &m[3][15]),
            load(&m[4][12], &m[5][12]),   load(&m[4][13], &m[5][13]),
            load(&m[4][14], &m[5][14]),   load(&m[4][15], &m[5][15]),
            load(&m[6][12], &m[7][12]),   load(&m[6][13], &m[7][13]),
            load(&m[6][14], &m[7][14]),   load(&m[6][15], &m[7][15]),
            load(&m[8][12], &m[9][12]),   load(&m[8][13], &m[9][13]),
            load(&m[8][14], &m[9][14]),   load(&m[8][15], &m[9][15]),
            load(&m[10][12], &m[11][12]), load(&m[10][13], &m[11][13]),
            load(&m[10][14], &m[11][14]), load(&m[10][15], &m[11][15]),
            load(&m[12][12], &m[13][12]), load(&m[12][13], &m[13][13]),
            load(&m[12][14], &m[13][14]), load(&m[12][15], &m[13][15]),
            load(&m[14][12], &m[15][12]), load(&m[14][13], &m[15][13]),
            load(&m[14][14], &m[15][14]), load(&m[14][15], &m[15][15])};
     __m256d mmt[128];

     __m256d neg = _mm256_setr_pd(1.0, -1.0, 1.0, -1.0);
     for (unsigned i = 0; i < 128; ++i) {
          mmt[i] = _mm256_mul_pd(_mm256_permute_pd(mm[i], 5), neg);
     }
     std::size_t dsorted[] = {d0, d1, d2, d3};
     std::sort(dsorted, dsorted + 4, std::greater<std::size_t>());
     const auto m0 = dsorted[3] - 1;
     const auto m1 = (dsorted[2] >> 1) - dsorted[3];
     const auto m2 = (dsorted[1] >> 2) - (dsorted[2] >> 1);
     const auto m3 = (dsorted[0] >> 3) - (dsorted[1] >> 2);
     const auto m4 = (n >> 4) - (dsorted[0] >> 3);
     if (ctrlmask == 0) {
#pragma omp for collapse(1) schedule(static)
          for (std::size_t i = 0; i<n>> 4; i += buf_sz) {
               kernel_compute(v, i, d0, d1, d2, d3, mm, mmt, m0, m1, m2, m3,
                              m4);
          }
     }
     else {
#pragma omp for collapse(1) schedule(static)
          for (std::size_t i = 0; i<n>> 4; i += buf_sz) {
               kernel_compute(v, i, d0, d1, d2, d3, mm, mmt, m0, m1, m2, m3, m4,
                              ctrlmask);
          }
     }
}

template <class V, class M,
          void K(V &, unsigned, unsigned, unsigned, unsigned, M const &,
                 std::size_t)>
inline void kernelK(V &v, unsigned id3, unsigned id2, unsigned id1,
                    unsigned id0, M const &m, std::size_t ctrlmask)
{
     K(v, id3, id2, id1, id0, m, ctrlmask);
}
}  // namespace intrin_cf
