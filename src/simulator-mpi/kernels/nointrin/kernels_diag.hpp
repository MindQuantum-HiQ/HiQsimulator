#ifndef NOINTRIN_KERNELDIAG_HPP
#define NOINTRIN_KERNELDIAG_HPP

#include <cstdint>

namespace nointrin {

template <class V, class T>
void kernelK_diag1(V &psi, const T& d)
{
    std::size_t n = psi.size();

    #pragma omp for schedule(static)
    for (std::size_t i0 = 0; i0 < n; ++i0) {
        psi[i0] *= d;
    }
}

template <class V, class M>
inline void kernel_core_diag(V &psi, std::size_t I, std::size_t d0, M const& m)
{
    psi[I] = mul(psi[I], m[0][0]);
    psi[I + d0] = mul(psi[I + d0], m[1][1]);

}

template <class V, class M>
inline void kernel_core_diag(V &psi, std::size_t I, std::size_t d0, std::size_t d1, M const& m)
{
    psi[I] = mul(psi[I], m[0][0]);
    psi[I + d0] = mul(psi[I + d0], m[1][1]);
    psi[I + d1] = mul(psi[I + d1], m[2][2]);
    psi[I + d0 + d1] = mul(psi[I + d0 + d1], m[3][3]);

}

template <class V, class M>
inline void kernel_core_diag(V &psi, std::size_t I, std::size_t d0, std::size_t d1, std::size_t d2, M const& m)
{
    psi[I] = mul(psi[I], m[0][0]);
    psi[I + d0] = mul(psi[I + d0], m[1][1]);
    psi[I + d1] = mul(psi[I + d1], m[2][2]);
    psi[I + d0 + d1] = mul(psi[I + d0 + d1], m[3][3]);
    psi[I + d2] = mul(psi[I + d2], m[4][4]);
    psi[I + d0 + d2] = mul(psi[I + d0 + d2], m[5][5]);
    psi[I + d1 + d2] = mul(psi[I + d1 + d2], m[6][6]);
    psi[I + d0 + d1 + d2] = mul(psi[I + d0 + d1 + d2], m[7][7]);

}

template <class V, class M>
inline void kernel_core_diag(V &psi, std::size_t I, std::size_t d0, std::size_t d1, std::size_t d2, std::size_t d3, M const& m)
{
    psi[I] = mul(psi[I], m[0][0]);
    psi[I + d0] = mul(psi[I + d0], m[1][1]);
    psi[I + d1] = mul(psi[I + d1], m[2][2]);
    psi[I + d0 + d1] = mul(psi[I + d0 + d1], m[3][3]);
    psi[I + d2] = mul(psi[I + d2], m[4][4]);
    psi[I + d0 + d2] = mul(psi[I + d0 + d2], m[5][5]);
    psi[I + d1 + d2] = mul(psi[I + d1 + d2], m[6][6]);
    psi[I + d0 + d1 + d2] = mul(psi[I + d0 + d1 + d2], m[7][7]);
    psi[I + d3] = mul(psi[I + d3], m[8][8]);
    psi[I + d0 + d3] = mul(psi[I + d0 + d3], m[9][9]);
    psi[I + d1 + d3] = mul(psi[I + d1 + d3], m[10][10]);
    psi[I + d0 + d1 + d3] = mul(psi[I + d0 + d1 + d3], m[11][11]);
    psi[I + d2 + d3] = mul(psi[I + d2 + d3], m[12][12]);
    psi[I + d0 + d2 + d3] = mul(psi[I + d0 + d2 + d3], m[13][13]);
    psi[I + d1 + d2 + d3] = mul(psi[I + d1 + d2 + d3], m[14][14]);
    psi[I + d0 + d1 + d2 + d3] = mul(psi[I + d0 + d1 + d2 + d3], m[15][15]);

}

template <class V, class M>
inline void kernel_core_diag(V &psi, std::size_t I, std::size_t d0, std::size_t d1, std::size_t d2, std::size_t d3, std::size_t d4, M const& m)
{
    psi[I] = mul(psi[I], m[0][0]);
    psi[I + d0] = mul(psi[I + d0], m[1][1]);
    psi[I + d1] = mul(psi[I + d1], m[2][2]);
    psi[I + d0 + d1] = mul(psi[I + d0 + d1], m[3][3]);
    psi[I + d2] = mul(psi[I + d2], m[4][4]);
    psi[I + d0 + d2] = mul(psi[I + d0 + d2], m[5][5]);
    psi[I + d1 + d2] = mul(psi[I + d1 + d2], m[6][6]);
    psi[I + d0 + d1 + d2] = mul(psi[I + d0 + d1 + d2], m[7][7]);
    psi[I + d3] = mul(psi[I + d3], m[8][8]);
    psi[I + d0 + d3] = mul(psi[I + d0 + d3], m[9][9]);
    psi[I + d1 + d3] = mul(psi[I + d1 + d3], m[10][10]);
    psi[I + d0 + d1 + d3] = mul(psi[I + d0 + d1 + d3], m[11][11]);
    psi[I + d2 + d3] = mul(psi[I + d2 + d3], m[12][12]);
    psi[I + d0 + d2 + d3] = mul(psi[I + d0 + d2 + d3], m[13][13]);
    psi[I + d1 + d2 + d3] = mul(psi[I + d1 + d2 + d3], m[14][14]);
    psi[I + d0 + d1 + d2 + d3] = mul(psi[I + d0 + d1 + d2 + d3], m[15][15]);
    psi[I + d4] = mul(psi[I + d4], m[16][16]);
    psi[I + d0 + d4] = mul(psi[I + d0 + d4], m[17][17]);
    psi[I + d1 + d4] = mul(psi[I + d1 + d4], m[18][18]);
    psi[I + d0 + d1 + d4] = mul(psi[I + d0 + d1 + d4], m[19][19]);
    psi[I + d2 + d4] = mul(psi[I + d2 + d4], m[20][20]);
    psi[I + d0 + d2 + d4] = mul(psi[I + d0 + d2 + d4], m[21][21]);
    psi[I + d1 + d2 + d4] = mul(psi[I + d1 + d2 + d4], m[22][22]);
    psi[I + d0 + d1 + d2 + d4] = mul(psi[I + d0 + d1 + d2 + d4], m[23][23]);
    psi[I + d3 + d4] = mul(psi[I + d3 + d4], m[24][24]);
    psi[I + d0 + d3 + d4] = mul(psi[I + d0 + d3 + d4], m[25][25]);
    psi[I + d1 + d3 + d4] = mul(psi[I + d1 + d3 + d4], m[26][26]);
    psi[I + d0 + d1 + d3 + d4] = mul(psi[I + d0 + d1 + d3 + d4], m[27][27]);
    psi[I + d2 + d3 + d4] = mul(psi[I + d2 + d3 + d4], m[28][28]);
    psi[I + d0 + d2 + d3 + d4] = mul(psi[I + d0 + d2 + d3 + d4], m[29][29]);
    psi[I + d1 + d2 + d3 + d4] = mul(psi[I + d1 + d2 + d3 + d4], m[30][30]);
    psi[I + d0 + d1 + d2 + d3 + d4] = mul(psi[I + d0 + d1 + d2 + d3 + d4], m[31][31]);

}

}

#endif //NOINTRIN_KERNELDIAG_HPP
