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

/*
 * funcs.hpp
 *
 *  Created on: Jan 30, 2018
 *      Author: zotov
 */

#ifndef _FUNCS_HPP_
#define _FUNCS_HPP_

#include <glog/logging.h>

#include <algorithm>
#include <bitset>
#include <boost/container/vector.hpp>
#include <boost/format.hpp>
#include <complex>
#include <cstdint>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>

using boost::format;
using std::cout;
using std::endl;

namespace bc = boost::container;

enum MatProps
{
     IS_DIAG = 1
};

namespace hiq
{
std::ostream& operator<<(std::ostream& out, const std::vector<bool>& v);

template <class Simulator>
void printAmplitude(const Simulator& sim, std::vector<bool>&& bits_string,
                    size_t bit)
{
     if (bit != 0) {
          bits_string[bit] = 0;
          printAmplitude(sim, std::move(bits_string), bit - 1);

          bits_string[bit] = 1;
          printAmplitude(sim, std::move(bits_string), bit - 1);
     }
     else {
          bits_string[bit] = 0;
          auto a = sim.GetAmplitude(bits_string, sim.GetQubitsPermutation());
          std::cerr << bits_string << ": " << a << endl;

          bits_string[bit] = 1;
          a = sim.GetAmplitude(bits_string, sim.GetQubitsPermutation());
          std::cerr << bits_string << ": " << a << endl;
     }
}

template <class Simulator>
void printAmplitudes(const Simulator& sim)
{
     printAmplitude(sim, std::vector<bool>(sim.TotalQubitsCount()),
                    sim.TotalQubitsCount() - 1);
}
}  // namespace hiq

template <class T>
struct Printer {};

template <class V>
struct PrintVector : public Printer<PrintVector<V>> {
     const V& v;
     PrintVector(const V& aSV) : v(aSV)
     {}

     template <class Stream>
     void do_out(Stream& out) const
     {
          for (size_t i = 0; i < v.size(); ++i) {
               out << format("%d:%.7g ") % i % v[i];
          }
     }
};

template <>
struct PrintVector<std::vector<bool>>
    : public Printer<PrintVector<std::vector<bool>>> {
     const std::vector<bool>& v;
     PrintVector(const std::vector<bool>& aSV) : v(aSV)
     {}

     template <class Stream>
     void do_out(Stream& out) const
     {
          for (size_t i = 0; i < v.size(); ++i) {
               out << format("%d:%.7g ") % i % double(v[i]);
          }
     }
};

template <class V>
struct PrintMap : public Printer<PrintMap<V>> {
     const V& v;
     PrintMap(const V& aSV) : v(aSV)
     {}

     template <class Stream>
     void do_out(Stream& out) const
     {
          for (auto& p: v) {
               out << format("%d->%s ") % p.first % p.second;
          }
     }
};

template <class V>
struct PrintPairs : public Printer<PrintPairs<V>> {
     const V& v;
     PrintPairs(const V& aSV) : v(aSV)
     {}

     template <class Stream>
     void do_out(Stream& out) const
     {
          for (size_t i = 0; i < v.size(); i += 2) {
               out << format("%d->%s ") % v[i] % v[i + 1];
          }
     }
};

template <class Stream, class T>
Stream& operator<<(Stream& out, const Printer<T>& p)
{
     static_cast<const T&>(p).do_out(out);
     return out;
}

template <class V, class A>
PrintVector<std::vector<V, A>> print(const std::vector<V, A>& v)
{
     return PrintVector<std::vector<V, A>>(v);
}

template <class V, class A>
PrintVector<bc::vector<V, A>> print(const bc::vector<V, A>& v)
{
     return PrintVector<bc::vector<V, A>>(v);
}

template <class V, class A>
PrintMap<std::map<V, A>> print(const std::map<V, A>& v)
{
     return PrintMap<std::map<V, A>>(v);
}

template <class V>
PrintPairs<std::vector<V>> printPairs(const std::vector<V>& v)
{
     return PrintPairs<std::vector<V>>(v);
}

template <class T>
std::vector<T> naturalOrdering(T nQubits)
{
     std::vector<T> res;
     res.reserve(nQubits);
     for (T i = 0; i < nQubits; ++i)
          res.push_back(i);

     return std::move(res);
}

template <class T, class T2>
std::vector<T> permuteQubits(std::vector<T>&& aPerm, T2 qubit1, T2 qubit2)
{
     auto it1 = std::find(aPerm.begin(), aPerm.end(), qubit1);
     auto it2 = std::find(aPerm.begin(), aPerm.end(), qubit2);

     std::swap(*it1, *it2);

     return std::move(aPerm);
}

template <class T, class T2>
std::vector<T> permuteQubits(std::vector<T>&& aPerm,
                             const std::vector<T2>& aPairs)
{
     std::vector<T> res = std::move(aPerm);
     for (size_t i = 0; i < aPairs.size(); i += 2) {
          res = permuteQubits(std::move(res), aPairs[i], aPairs[i + 1]);
     }

     return std::move(res);
}

template <class T>
std::vector<T> inversePermutation(const std::vector<T>& aPerm)
{
     std::vector<T> res(aPerm.size());

     for (size_t i = 0; i < aPerm.size(); ++i) {
          res[aPerm[i]] = i;
     }

     return std::move(res);
}

template <class T>
void inversePermutation_ng(bc::vector<T>& res, const std::vector<T>& aPerm)
{
     res.resize(aPerm.size(), bc::default_init);

     for (size_t i = 0; i < aPerm.size(); ++i) {
          res[aPerm[i]] = i;
     }
}

template <class T>
std::vector<typename T::value_type> applyPermutation(
    const T& aV, const std::vector<uint8_t>& aPerm)
{
     std::vector<typename T::value_type> res(aPerm.size());

     for (size_t i = 0; i < aPerm.size(); ++i) {
          res[i] = aV[aPerm[i]];
     }

     return std::move(res);
}

template <class T, class T1, class M>
std::vector<T> q2bits(uint64_t aM, const std::vector<T1>& aQbitPairs,
                      const M& anInversePerm)
{
     std::map<T1, T1> m;
     for (size_t i = 0; i < aQbitPairs.size(); i += 2) {
          m[anInversePerm[aQbitPairs[i + 1]]] =
              anInversePerm[aQbitPairs[i]] - aM;
     }

     std::vector<T> res(aQbitPairs.size());
     size_t i = 0;
     for (auto& p: m) {
          res[i] = (T(1) << p.second);
          res[i + 1] = (T(1) << p.first);
          i += 2;
     }

     return std::move(res);
}

template <class T, class T1, class M>
void q2bits_ng(std::vector<T>& res, const std::vector<T1>& aQbitPairs, M pos)
{
     uint64_t n_bits = aQbitPairs.size() / 2;
     res.resize(4 * n_bits);

     for (size_t i = 0; i < n_bits; ++i) {
          res[i] = 1ul << pos[aQbitPairs[2 * i + 1]];
          res[n_bits + i] = i;
          res[2 * n_bits + i] = 1ul << (pos[aQbitPairs[2 * i]]);
          res[3 * n_bits + i] = i;
     }

     DLOG(INFO) << "q2bits_ng(): " << print(res);

     // reorder bits permutation according to local bits order
     std::sort(res.begin() + 3 * n_bits, res.begin() + 4 * n_bits,
               [&](size_t i1, size_t i2) { return res[i1] < res[i2]; });

     // sort local bits, so the above permutation corresponds to this sorted
     // order
     std::sort(res.begin(), res.begin() + n_bits);

     // sort indices of local bits in order of remote bits
     std::sort(res.begin() + n_bits, res.begin() + 2 * n_bits,
               [&](size_t i1, size_t i2) {
                    auto ii1 = res[3 * n_bits + i1];
                    auto ii2 = res[3 * n_bits + i2];
                    return res[2 * n_bits + ii1] < res[2 * n_bits + ii2];
               });
}

template <class T>
T getColor(T aRank, const std::vector<T>& aSwapBits)
{
     T res = aRank;
     for (size_t i = 0; i < aSwapBits.size(); i += 2) {
          res &= ~aSwapBits[i];
     }

     return res;
}

template <class T>
T getColor_ng(uint64_t M, T aRank, const std::vector<T>& aQubitPairs,
              const bc::vector<T>& iperm)
{
     uint64_t n_bits = aQubitPairs.size() / 2;
     T res = aRank;
     for (size_t i = 0; i < n_bits; ++i) {
          res &= ~(1ul << (iperm[aQubitPairs[2 * i]] - M));
     }

     return res;
}

template <class M>
bool is_diag(const M& m, size_t rows, size_t cols)
{
     for (size_t i = 0; i < rows; ++i) {
          for (size_t j = 0; j < cols; ++j) {
               if (i != j && m[i][j] != 0.0) {
                    return false;
               }
          }
     }

     return true;
}

template <class M>
uint64_t get_matrix_props(const M& m, size_t rows, size_t cols)
{
     uint64_t flags = 0;

     flags |= (uint64_t) is_diag(m, rows, cols);

     return flags;
}

template <class Stream, class T>
void printVector(Stream& out, const char* name, const std::vector<T>& aSV)
{
     out << name << endl;
     for (size_t i = 0; i < aSV.size(); ++i) {
          out << format("%5d: %s") % i % aSV[i] << endl;
     }
}

template <class T>
std::string bitstring(T val, uint64_t L)
{
     return std::bitset<sizeof(T) * 8>(val).to_string().substr(
         sizeof(T) * 8 - L, L);
}

template <class Stream, class T>
void printStateVector(Stream& out, const char* name, uint64_t L,
                      const std::vector<T>& aSV)
{
     for (size_t i = 0; i < aSV.size(); ++i) {
          out << format("%5d: %s") % i % bitstring(aSV[i], L) << endl;
     }
}

template <class V>
void printStateVector1(const V& aSV)
{
     for (size_t i = 0; i < aSV.size(); ++i) {
          cout << format("%5d: %f") % i % aSV[i] << endl;
     }
}

template <class Stream, class T>
void printPairs(Stream& out, const char* name, const std::vector<T>& aPairs)
{
     out << name << endl;
     for (size_t i = 0; i < aPairs.size(); i += 2) {
          out << format("%d->%d") % aPairs[i] % aPairs[i + 1] << endl;
     }
}

template <class Stream, class K, class V>
void printMap(Stream& out, const char* name, const std::map<K, V>& aPairs)
{
     out << name << endl;
     for (auto& p: aPairs) {
          out << format("[%d: %d]") % p.first % p.second << endl;
     }
}

template <class Iterator>
double norm(const Iterator& begin, const Iterator& end)
{
     double res = 0.0;
     size_t sz = end - begin;

#pragma omp parallel for reduction(+ : res) schedule(static)
     for (size_t i = 0; i < sz; ++i) {
          res += std::norm(begin[i]);
     }

     return res;
}

#endif /* _FUNCS_HPP_ */
