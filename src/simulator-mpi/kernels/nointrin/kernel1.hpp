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

namespace nointrin
{
template <class V, class M>
inline void kernel_core(V& psi, std::size_t I, std::size_t d0, M const& m)
{
     const auto v0 = psi[I];

     psi[I] = (add(mul(v0, m[0][0]), mul(psi[I + d0], m[0][1])));
     psi[I + d0] = (add(mul(v0, m[1][0]), mul(psi[I + d0], m[1][1])));
}

// bit indices id[.] are given from high to low (e.g. control first for CNOT)
template <class V, class M>
void kernel(V& psi, unsigned id0, M const& m, std::size_t ctrlmask)
{
     const auto n = psi.size();
     const std::size_t d0 = 1UL << id0;
     const auto dsorted = d0;

     if (ctrlmask == 0) {
#pragma omp for collapse(LOOP_COLLAPSE1) schedule(static)
          for (std::size_t i0 = 0; i0 < n; i0 += 2 * dsorted) {
               for (std::size_t i1 = 0; i1 < dsorted; ++i1) {
                    kernel_core(psi, i0 + i1, d0, m);
               }
          }
     }
     else {
#pragma omp for collapse(LOOP_COLLAPSE1) schedule(static)
          for (std::size_t i0 = 0; i0 < n; i0 += 2 * dsorted) {
               for (std::size_t i1 = 0; i1 < dsorted; ++i1) {
                    if (((i0 + i1) & ctrlmask) == ctrlmask)
                         kernel_core(psi, i0 + i1, d0, m);
               }
          }
     }
}

// bit indices id[.] are given from high to low (e.g. control first for CNOT)
template <class V, class M, void K(V&, std::size_t, std::size_t, const M&)>
void kernelK(V& psi, unsigned id0, M const& m, std::size_t ctrlmask)
{
     const auto n = psi.size();
     const std::size_t d0 = 1UL << id0;
     const auto dsorted = d0;

     if (ctrlmask == 0) {
#pragma omp for collapse(LOOP_COLLAPSE1) schedule(static)
          for (std::size_t i0 = 0; i0 < n; i0 += 2 * dsorted) {
               for (std::size_t i1 = 0; i1 < dsorted; ++i1) {
                    K(psi, i0 + i1, d0, m);
               }
          }
     }
     else {
#pragma omp for collapse(LOOP_COLLAPSE1) schedule(static)
          for (std::size_t i0 = 0; i0 < n; i0 += 2 * dsorted) {
               for (std::size_t i1 = 0; i1 < dsorted; ++i1) {
                    if (((i0 + i1) & ctrlmask) == ctrlmask)
                         K(psi, i0 + i1, d0, m);
               }
          }
     }
}

}  // namespace nointrin
