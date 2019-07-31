#ifndef INTRIN_KERNELDIAG_HPP
#define INTRIN_KERNELDIAG_HPP

#include <immintrin.h>

#include <cstdint>

#include "cintrin.hpp"
#ifndef _mm256_set_m128d
#     define _mm256_set_m128d(hi, lo)                                          \
          _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), (hi), 0x1)
#endif
#ifndef _mm256_loadu2_m128d
#     define _mm256_loadu2_m128d(hiaddr, loaddr)                               \
          _mm256_set_m128d(_mm_loadu_pd(hiaddr), _mm_loadu_pd(loaddr))
#endif

namespace intrin
{
template <class V, class T>
void kernelK_diag1(V& v, const T& d)
{
     std::size_t n = v.size();
     __m256d c = load(&d, &d),
             c_t = _mm256_mul_pd(_mm256_permute_pd(c, 5),
                                 _mm256_setr_pd(1.0, -1.0, 1.0, -1.0));

#pragma omp for schedule(static)
     for (size_t i = 0; i < n; i += 2) {
          store(&v[i], &v[i + 1], mul(load(&v[i], &v[i + 1]), c, c_t));
     }
}

template <class V, class M>
inline void kernel_core_diag(V& v, unsigned id0, M const& m, std::size_t cmask)
{
     std::size_t n = v.size();
     __m256d neg = _mm256_setr_pd(1.0, -1.0, 1.0, -1.0);
     __m256d dv[2], dvt[2];
     for (unsigned i = 0; i < 2; ++i) {
          dv[i] = load2(&m[i][i]);
          dvt[i] = _mm256_mul_pd(_mm256_permute_pd(dv[i], 5), neg);
     }
#pragma omp for collapse(1) schedule(static)
     for (size_t i = 0; i < n; ++i) {
          if ((i & cmask) == cmask) {
               std::size_t d_id = ((i >> id0) & 1);
               store(&v[i], &v[i],
                     mul(load(&v[i], &v[i]), dv[d_id], dvt[d_id]));
          }
     }
}

template <class V, class M>
inline void kernel_core_diag(V& v, unsigned id1, unsigned id0, M const& m,
                             std::size_t cmask)
{
     const auto n = v.size();
     __m256d neg = _mm256_setr_pd(1.0, -1.0, 1.0, -1.0);

     __m256d dv[4], dvt[4];
     for (unsigned i = 0; i < 4; ++i) {
          dv[i] = load(&m[i][i], &m[i][i]);
          dvt[i] = _mm256_mul_pd(_mm256_permute_pd(dv[i], 5), neg);
     }
#pragma omp for collapse(1) schedule(static)
     for (size_t i = 0; i < n; ++i) {
          if ((i & cmask) == cmask) {
               std::size_t d_id = ((i >> id0) & 1) ^ (((i >> id1) & 1) << 1);
               store(&v[i], &v[i],
                     mul(load(&v[i], &v[i]), dv[d_id], dvt[d_id]));
          }
     }
}

template <class V, class M>
inline void kernel_core_diag(V& v, unsigned id2, unsigned id1, unsigned id0,
                             M const& m, std::size_t cmask)
{
     const auto n = v.size();
     __m256d neg = _mm256_setr_pd(1.0, -1.0, 1.0, -1.0);
     __m256d dv[8], dvt[8];
     for (unsigned i = 0; i < 8; ++i) {
          dv[i] = load(&m[i][i], &m[i][i]);
          dvt[i] = _mm256_mul_pd(_mm256_permute_pd(dv[i], 5), neg);
     }
#pragma omp for collapse(1) schedule(static)
     for (size_t i = 0; i < n; ++i) {
          if ((i & cmask) == cmask) {
               std::size_t d_id = ((i >> id0) & 1) ^ (((i >> id1) & 1) << 1)
                                  ^ (((i >> id2) & 1) << 2);
               store(&v[i], &v[i],
                     mul(load(&v[i], &v[i]), dv[d_id], dvt[d_id]));
          }
     }
}

template <class V, class M>
inline void kernel_core_diag(V& v, unsigned id3, unsigned id2, unsigned id1,
                             unsigned id0, M const& m, std::size_t cmask)
{
     const auto n = v.size();
     __m256d neg = _mm256_setr_pd(1.0, -1.0, 1.0, -1.0);
     __m256d dv[16], dvt[16];
     for (unsigned i = 0; i < 16; ++i) {
          dv[i] = load(&m[i][i], &m[i][i]);
          dvt[i] = _mm256_mul_pd(_mm256_permute_pd(dv[i], 5), neg);
     }
#pragma omp for collapse(1) schedule(static)
     for (size_t i = 0; i < n; ++i) {
          if ((i & cmask) == cmask) {
               std::size_t d_id = ((i >> id0) & 1) ^ (((i >> id1) & 1) << 1)
                                  ^ (((i >> id2) & 1) << 2)
                                  ^ (((i >> id3) & 1) << 3);
               store(&v[i], &v[i],
                     mul(load(&v[i], &v[i]), dv[d_id], dvt[d_id]));
          }
     }
}

template <class V, class M>
inline void kernel_core_diag(V& v, unsigned id4, unsigned id3, unsigned id2,
                             unsigned id1, unsigned id0, M const& m,
                             std::size_t cmask)
{
     const auto n = v.size();
     __m256d neg = _mm256_setr_pd(1.0, -1.0, 1.0, -1.0);
     __m256d dv[32], dvt[32];
     for (unsigned i = 0; i < 32; ++i) {
          dv[i] = load(&m[i][i], &m[i][i]);
          dvt[i] = _mm256_mul_pd(_mm256_permute_pd(dv[i], 5), neg);
     }
#pragma omp for collapse(1) schedule(static)
     for (size_t i = 0; i < n; ++i) {
          if ((i & cmask) == cmask) {
               std::size_t d_id = ((i >> id0) & 1) ^ (((i >> id1) & 1) << 1)
                                  ^ (((i >> id2) & 1) << 2)
                                  ^ (((i >> id3) & 1) << 3)
                                  ^ (((i >> id4) & 1) << 4);
               store(&v[i], &v[i],
                     mul(load(&v[i], &v[i]), dv[d_id], dvt[d_id]));
          }
     }
}
}  // namespace intrin

#endif  // INTRIN_KERNELDIAG_HPP
