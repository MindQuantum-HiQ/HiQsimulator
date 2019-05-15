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

#include <pybind11/pybind11.h>
#include <x86intrin.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <tuple>
#include <vector>

#include "simulator-mpi/alignedallocator.hpp"

/* For some weird reason, throwing std::runtime_error leads to segmentation
 * faults with pybind11 2.2.4 & clang 10.0
 */
// #define PYBIND11_THROW_RUNTIME_ERROR
#ifdef PYBIND11_THROW_RUNTIME_ERROR
#     define THROW_RUNTIME_ERROR(msg) throw std::runtime_error(msg)
#else
#     define THROW_RUNTIME_ERROR(msg)               \
          PyErr_SetString(PyExc_RuntimeError, msg); \
          throw pybind11::error_already_set()
#endif /* PYBIND11_THROW_RUNTIME_ERROR */

#define QUBIT_ALLOCATE_CHECK(qubit_id, gate_name)                \
     if (allocated_qubits_.count(qubit_id) == 0) {               \
          THROW_RUNTIME_ERROR("Error (" gate_name                \
                              "): Qubit ID is not "              \
                              "valid! (ie. unallocated Qubit)"); \
     }

constexpr auto max_storage_depth = 10U;

using AlignedIntVec = std::vector<uint32_t, aligned_allocator<uint32_t, 256>>;
using VecOfMasks = std::vector<AlignedIntVec>;
using VecOfCNOT = std::vector<std::vector<std::pair<unsigned, unsigned>>>;

using std::begin;
using std::end;

struct Gates {
    public:
     Gates(unsigned n = 1, unsigned max_d_a = 1)
         : num_gates(0),
           max_d(max_d_a),
           nmasks_8(8 * std::ceil(std::ceil(n / 32.) / 8.)),
           H(max_d * nmasks_8, 0),
           S(max_d * nmasks_8, 0),
           C(max_d)
     {
          for (auto& c: C) {
               c.reserve(max_storage_depth);
          }
     }

     void reset()
     {
          std::fill(begin(H), end(H), 0);
          std::fill(begin(S), end(S), 0);

          num_gates = 0;
          for (auto& c: C) {
               c.clear();
          }
     }

     unsigned num_gates;
     const unsigned max_d;
     const unsigned nmasks_8;

     AlignedIntVec H;
     AlignedIntVec S;
     VecOfCNOT C;
};

class StabilizerSimulator
{
     using calc_type = double;
     using Vec = std::vector<unsigned>;
     using Map = std::map<unsigned, unsigned>;

    public:
     StabilizerSimulator(unsigned num_qubits, unsigned seed)
         : num_qubits_(num_qubits)
           // number of 32-bit integers for one row/stabilizer generator
           ,
           row_length_(std::ceil(std::ceil(num_qubits / 32.) / 8.) * 8)
           // Saves x and z in a vertical order
           ,
           x_(row_length_ * (2 * num_qubits + 1), 0),
           z_(row_length_ * (2 * num_qubits + 1), 0),
           r_(std::ceil((2 * num_qubits + 1) / 32.), 0),
           rnd_eng_(seed),
           dist_(0, 1),
           gates_(num_qubits, max_storage_depth),
           pos_(num_qubits, 0),
           allocated_qubits_(),
           available_qubits_(num_qubits)
     {
          // Init all qubits to state 0
#pragma omp parallel for schedule(static, 32)
          for (auto i = 0U; i < 2 * num_qubits; ++i) {
               if (i < num_qubits) {
                    set_stab_x_(i, i, true);
               }
               else {
                    set_stab_z_(i, i - num_qubits, true);
               }
          }

#if __cplusplus < 201402L
          auto n(num_qubits_);
          std::generate(begin(available_qubits_), end(available_qubits_),
                        [&n]() { return --n; });
#else
          std::generate(begin(available_qubits_), end(available_qubits_),
                        [n = num_qubits_]() mutable { return --n; });
#endif
     }

     // =========================================================================
     // Qubit allocation/deallocation methods

     void allocate_qubit(unsigned qubit_id)
     {
          sync();
          if (allocated_qubits_.count(qubit_id) == 0) {
               if (available_qubits_.empty()) {
                    THROW_RUNTIME_ERROR(
                        "AllocateQubit: cannot allocate Qubit. "
                        "No more available IDs.");
               }

               allocated_qubits_[qubit_id] = available_qubits_.back();
               available_qubits_.pop_back();
          }
          else {
               THROW_RUNTIME_ERROR(
                   "AllocateQubit: ID already exists. "
                   "Qubit IDs should be unique.");
          }
     }

     void allocate_all_qubits()
     {
          for (auto id(0UL); id < num_qubits_; ++id) {
               allocate_qubit(id);
          }
     }

     void deallocate_qubit(unsigned qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "DeallocateGate");

          sync();
          const auto qubit_index(allocated_qubits_[qubit_id]);
          assert(allocated_qubits_.count(qubit_id) == 1);
          const auto prob(get_probability({false}, {qubit_id}));
          if (prob == 0.0 || prob == 1.0) {
               // Make sure that the qubit is returned to state |0>
               if (prob == 0.0) {
                    X_(qubit_index);
                    sync();
               }
               available_qubits_.push_back(qubit_index);
               allocated_qubits_.erase(qubit_id);
          }
          else {
               THROW_RUNTIME_ERROR(
                   "Error: Qubit has not been measured / "
                   "uncomputed! There is most likely a "
                   "bug in your code.");
          }
     }

     // =========================================================================
     // Quantum gates

     void H(unsigned qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "HGate");
          H_(allocated_qubits_[qubit_id]);
     }

     void S(unsigned qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "SGate");
          S_(allocated_qubits_[qubit_id]);
     }

     void X(unsigned qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "XGate");
          X_(allocated_qubits_[qubit_id]);
     }

     void CNOT(unsigned control_qubit_id, unsigned target_qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(control_qubit_id, "CNOTGate");
          QUBIT_ALLOCATE_CHECK(target_qubit_id, "CNOTGate");
          CNOT_(allocated_qubits_[control_qubit_id],
                allocated_qubits_[target_qubit_id]);
     }

     // =========================================================================
     // Misc. ProjectQ utility functions

     std::vector<bool> measure_qubits_return(
         std::vector<unsigned> const& qubit_ids)
     {
          sync();
          std::vector<bool> ret;
          std::transform(
              begin(qubit_ids), end(qubit_ids), std::back_inserter(ret),
              [&](const unsigned& qubit_id) {
                   QUBIT_ALLOCATE_CHECK(qubit_id, "MeasureGate");
                   return this->measure_(allocated_qubits_[qubit_id]);
              });
          return ret;
     }

     bool measure(unsigned qubit_id)
     {
          sync();
          QUBIT_ALLOCATE_CHECK(qubit_id, "MeasureGate");
          return measure_(allocated_qubits_[qubit_id]);
     }

     bool get_classical_value(unsigned qubit_id, calc_type tol = 1.e-12)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "is_classical");
          sync();
          const auto prob = get_probability({false}, {qubit_id});
          // prob is one of {0, 1, 0.5}
          if (std::abs(prob - 0.5) < tol) {
               THROW_RUNTIME_ERROR(
                   "Qubit has not been measured / "
                   "uncomputed. Cannot access its "
                   "classical value and/or deallocate a "
                   "qubit in superposition!");
          }
          else {
               // prob = either 0 (qubit up) or 1 (qubit down)
               return prob < tol;
          }
     }

     bool is_classical(unsigned qubit_id, calc_type tol = 1.e-12)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "is_classical");
          sync();
          const auto prob = get_probability({false}, {qubit_id});
          // prob is one of {0, 1, 0.5}
          if (std::abs(prob - 0.5) < tol) {
               return false;
          }
          else {
               return true;
          }
     }

     double get_probability(const std::vector<bool>& bitstring,
                            const std::vector<unsigned>& qubit_ids)
     {
          sync();
          if (bitstring.empty()) {
               return 1.0;
          }
          assert(bitstring.size() == qubit_ids.size());
          return get_probability_recursive_(begin(bitstring), end(bitstring),
                                            begin(qubit_ids), end(qubit_ids));
     }

     void collapse_wavefunction(const std::vector<unsigned>& qubit_ids,
                                const std::vector<bool>& bitstring)
     {
          sync();
          if (bitstring.empty()) {
               return;
          }
          assert(qubit_ids.size() == bitstring.size());
          auto qubit_it(begin(qubit_ids));
          auto bitstring_it(begin(bitstring));
          const auto qubit_end(end(qubit_ids));

          const auto r_bak = r_;
          const auto x_bak = x_;
          const auto z_bak = z_;

          try {
               for (; qubit_it != qubit_end; ++qubit_it, ++bitstring_it) {
                    QUBIT_ALLOCATE_CHECK(*qubit_it, "collapse_wavefunction");
                    collapse_wavefunction_(allocated_qubits_[*qubit_it],
                                           *bitstring_it);
               }
          }
          catch (pybind11::error_already_set& e) {
               r_ = r_bak;
               x_ = x_bak;
               z_ = z_bak;
               throw;
          }
     }

     void set_qubits(const std::vector<unsigned>& qubit_ids,
                     const std::vector<bool>& bitstring)
     {
          sync();
          if (bitstring.empty()) {
               return;
          }
          assert(qubit_ids.size() == bitstring.size());
          auto qubit_it(begin(qubit_ids));
          auto bitstring_it(begin(bitstring));
          const auto qubit_end(end(qubit_ids));
          std::vector<unsigned> qubit_indices_to_invert;

          for (; qubit_it != qubit_end; ++qubit_it, ++bitstring_it) {
               QUBIT_ALLOCATE_CHECK(*qubit_it, "set_qubits");
               if (set_qubits_(allocated_qubits_[*qubit_it], *bitstring_it)) {
                    qubit_indices_to_invert.emplace_back(
                        allocated_qubits_[*qubit_it]);
               }
          }
          for (const auto& qubit_index: qubit_indices_to_invert) {
               X_(qubit_index);
          }
     }

     // =========================================================================
     // Misc. methods

     void sync()
     {
          if (gates_.num_gates == 0)
               return;
#pragma omp parallel for schedule(static, 32)
          for (auto stab = 0U; stab < 2 * num_qubits_; ++stab) {
               const auto& nmasks_8 = gates_.nmasks_8;
               for (auto d = 0U; d < gates_.max_d; ++d) {
                    h_and_s_kernel_(begin(gates_.H) + d * nmasks_8,
                                    begin(gates_.H) + (d + 1) * nmasks_8,
                                    begin(gates_.S) + d * nmasks_8,
                                    begin(gates_.S) + (d + 1) * nmasks_8, stab);
                    for (const auto& cnot_tuple: gates_.C[d]) {
                         cnot_kernel_(cnot_tuple.first, cnot_tuple.second,
                                      stab);
                    }
               }
          }
          gates_.reset();
          std::fill(begin(pos_), end(pos_), 0);
     }

     friend std::ostream& operator<<(std::ostream& out,
                                     StabilizerSimulator const& sim);

    private:
     // index for x, z
     unsigned get_index_(unsigned stab, unsigned qubit) const
     {
          return stab * row_length_ + (qubit >> 5);
     }

     bool get_stab_x_(unsigned stab, unsigned qubit) const
     {
          return (x_[get_index_(stab, qubit)] >> (qubit & 31)) & 1;
     }

     bool get_stab_z_(unsigned stab, unsigned qubit) const
     {
          return (z_[get_index_(stab, qubit)] >> (qubit & 31)) & 1;
     }

     bool get_phase_bit_(unsigned stab) const
     {
          return (r_[stab >> 5] >> (stab & 31)) & 1;
     }

     // used for storage independent testing
     void toggle_stab_x_(unsigned stab, unsigned qubit)
     {
          x_[get_index_(stab, qubit)] ^= (1ULL << (qubit & 31));
     }

     void set_stab_x_(unsigned stab, unsigned qubit, bool value)
     {
          if (get_stab_x_(stab, qubit) != value)
               toggle_stab_x_(stab, qubit);
     }

     void toggle_stab_z_(unsigned stab, unsigned qubit,
                         unsigned long long one = 1ULL)
     {
          z_[get_index_(stab, qubit)] ^= (one << (qubit & 31));
     }

     void set_stab_z_(unsigned stab, unsigned qubit, bool value)
     {
          if (get_stab_z_(stab, qubit) != value)
               toggle_stab_z_(stab, qubit);
     }

     void toggle_phase_bit_(unsigned stab)
     {
          r_[stab >> 5] ^= 1ULL << (stab & 31);
     }

     void set_phase_bit_(unsigned stab, bool value)
     {
          if (get_phase_bit_(stab) != value)
               toggle_phase_bit_(stab);
     }

     unsigned compute_phase_(unsigned stabilizer_h, unsigned stabilizer_i) const
     {
          // only inner loop: 22 int ops and 4 * 4byte rw

          // phase is i^exponent, where exponent is saved as a 2 bit number
          auto exp_low = _mm256_setzero_si256();
          auto exp_high1 = _mm256_setzero_si256();
          auto exp_high2 = _mm256_setzero_si256();
          const auto start_index_h = stabilizer_h * row_length_;
          const auto start_index_i = stabilizer_i * row_length_;
          auto ones = _mm256_set1_epi32((1ULL << 32) - 1);
          for (unsigned i = 0; i < row_length_; i += 8) {
               auto xh = _mm256_load_si256((__m256i*) &x_[start_index_h + i]);
               auto xi = _mm256_load_si256((__m256i*) &x_[start_index_i + i]);
               auto zh = _mm256_load_si256((__m256i*) &z_[start_index_h + i]);
               auto zi = _mm256_load_si256((__m256i*) &z_[start_index_i + i]);
               auto not_xh = _mm256_xor_si256(ones, xh);
               auto not_zi = _mm256_xor_si256(ones, zi);
               auto q = _mm256_and_si256(
                   _mm256_and_si256(_mm256_or_si256(xi, zh),
                                    _mm256_or_si256(not_xh, zi)),
                   _mm256_or_si256(not_zi, xh));
               auto r = _mm256_and_si256(_mm256_or_si256(xi, zi),
                                         _mm256_or_si256(xh, zh));
               auto s = _mm256_or_si256(_mm256_xor_si256(xi, xh),
                                        _mm256_xor_si256(zi, zh));
               auto r_and_s = _mm256_and_si256(r, s);
               exp_high1 = _mm256_xor_si256(_mm256_and_si256(exp_low, r_and_s),
                                            exp_high1);
               exp_high2 =
                   _mm256_xor_si256(exp_high2, _mm256_and_si256(q, r_and_s));
               exp_low = _mm256_xor_si256(exp_low, r_and_s);
               /*
                 uint32_t stab_h_x = x[start_index_h + i];
                 uint32_t stab_h_z = z[start_index_h + i];
                 uint32_t stab_i_x = x[start_index_i + i];
                 uint32_t stab_i_z = z[start_index_i + i];

                 uint32_t q = (stab_i_x | stab_h_z) & (stab_i_z | ~stab_h_x)
                 & (~stab_i_z | stab_h_x);
                 uint32_t r = (stab_i_x | stab_i_z) & (stab_h_x | stab_h_z);
                 uint32_t s = ~(~(stab_i_x ^ stab_h_x)
                 & ~(stab_i_z ^ stab_h_z));
                 uint32_t r_and_s = r & s;
                 exp_high = (exp_low & r_and_s) ^ (exp_high ^ (q & r_and_s));
                 exp_low ^= r_and_s;
               */
          }
          const auto exp_high = _mm256_xor_si256(exp_high1, exp_high2);
          alignas(32) uint64_t eh[4], el[4];
          _mm256_store_si256((__m256i*) &eh, exp_high);
          _mm256_store_si256((__m256i*) &el, exp_low);

          eh[0] ^= eh[1];
          eh[2] ^= eh[3];
          el[0] ^= el[1];
          el[2] ^= el[3];

          eh[0] ^= eh[2];
          el[0] ^= el[2];
          uint32_t mask = (1ULL << 32) - 1;
          uint32_t eh_sm = ((eh[0] >> 32) & mask) ^ (eh[0] & mask);
          uint32_t el_sm = ((el[0] >> 32) & mask) ^ (el[0] & mask);
          const auto exponent = 2 * _popcnt32(eh_sm) + _popcnt32(el_sm);
          // compute positive modulo.
          return (((exponent + 2 * get_phase_bit_(stabilizer_h) +
                    2 * get_phase_bit_(stabilizer_i)) %
                   4) +
                  4) %
                 4;
     }

     // Left-multiply stabilizer generator h by stabilizer generator i,
     // i.e., h <- i * h
     // For all qubits j: x_hj ^= x_ij and z_hj ^= z_ij
     void rowsum_(unsigned stabilizer_h, unsigned stabilizer_i)
     {
          // phase is either 0 (+1) or 2 (-1) which we map to 0 (+1) and
          // 1 (-1) as before:
          set_phase_bit_(stabilizer_h,
                         compute_phase_(stabilizer_h, stabilizer_i) / 2);
          const auto start_h = stabilizer_h * row_length_;
          const auto start_i = stabilizer_i * row_length_;

          for (auto i(0U); i < row_length_; i += 8) {
               // x_hj ^= x_ij
               // x[start_h + i] ^= x[start_i + i];
               const auto xh = _mm256_load_si256((__m256i*) &x_[start_h + i]);
               const auto xi = _mm256_load_si256((__m256i*) &x_[start_i + i]);
               _mm256_store_si256((__m256i*) &x_[start_h + i],
                                  _mm256_xor_si256(xh, xi));
               // same for z_hj:
               // z[start_h + i] ^= z[start_i + i];
               const auto zh = _mm256_load_si256((__m256i*) &z_[start_h + i]);
               const auto zi = _mm256_load_si256((__m256i*) &z_[start_i + i]);
               _mm256_store_si256((__m256i*) &z_[start_h + i],
                                  _mm256_xor_si256(zh, zi));
          }
     }

     inline void h_and_s_kernel_(AlignedIntVec::const_iterator Hmasks_begin,
                                 AlignedIntVec::const_iterator Hmasks_end,
                                 AlignedIntVec::const_iterator Smasks_begin,
                                 AlignedIntVec::const_iterator Smasks_end,
                                 unsigned stab)
     {
          auto phase_sum = _mm256_setzero_si256();
          auto i(row_length_ * stab);
          for (auto h_it(Hmasks_begin), s_it(Smasks_begin); h_it < Hmasks_end;
               h_it += 8, s_it += 8, i += 8) {
               const auto hmask = _mm256_load_si256((__m256i*) &(*h_it));
               const auto smask = _mm256_load_si256((__m256i*) &(*s_it));
               auto xstab = _mm256_load_si256((__m256i*) &x_[i]);
               auto zstab = _mm256_load_si256((__m256i*) &z_[i]);
               const auto smasked_x = _mm256_and_si256(xstab, smask);
               const auto smasked_x_and_z = _mm256_and_si256(zstab, smasked_x);
               // keep track of phase for s-gate
               phase_sum = _mm256_xor_si256(smasked_x_and_z, phase_sum);
               // update zstab for s-gate
               zstab = _mm256_xor_si256(smasked_x, zstab);

               auto hmasked_z = _mm256_and_si256(zstab, hmask);
               auto hmasked_x_and_z = _mm256_and_si256(hmasked_z, xstab);
               // update phase sum for H-gate
               phase_sum = _mm256_xor_si256(phase_sum, hmasked_x_and_z);
               // apply H-gate
               xstab = _mm256_xor_si256(zstab, xstab);
               auto hmasked_x = _mm256_and_si256(xstab, hmask);
               zstab = _mm256_xor_si256(hmasked_x, zstab);
               xstab = _mm256_xor_si256(zstab, xstab);

               _mm256_store_si256((__m256i*) &x_[i], xstab);
               _mm256_store_si256((__m256i*) &z_[i], zstab);
          }
          // reduce & store phase sum (for s-gate)
          alignas(32) uint64_t tmpvec[4];
          _mm256_store_si256((__m256i*) &tmpvec, phase_sum);
          const uint64_t r1 = tmpvec[0] ^ tmpvec[1];
          const uint64_t r2 = tmpvec[2] ^ tmpvec[3];
          const uint32_t mask = -1;
          const uint64_t red1 = r1 ^ r2;
          uint32_t red2 = ((red1 >> 32) & mask) ^ (red1 & mask);
          set_phase_bit_(stab, get_phase_bit_(stab) ^ (_popcnt32(red2) & 1));
     }

     inline void cnot_kernel_(unsigned qubit1, unsigned qubit2, unsigned stab)
     {
          // 7 int ops and 8 * 4bytes rw
          const auto z1 = get_stab_z_(stab, qubit1);
          const auto z2 = get_stab_z_(stab, qubit2);
          toggle_stab_z_(stab, qubit1, z2);
          const auto x1 = get_stab_x_(stab, qubit1);
          const auto x2 = get_stab_x_(stab, qubit2);
          set_phase_bit_(stab, get_phase_bit_(stab) ^ (x1 & z2 & (x2 == z1)));
          set_stab_x_(stab, qubit2, x2 ^ x1);
     }

     void H_(unsigned qubit_index)
     {
          unsigned mask_num = qubit_index / 32;
          gates_.H[pos_[qubit_index] * gates_.nmasks_8 + mask_num] |=
              (1 << (qubit_index - mask_num * 32));
          ++pos_[qubit_index];
          ++gates_.num_gates;
          if (pos_[qubit_index] >= max_storage_depth) {
               sync();
          }
     }

     void S_(unsigned qubit_index)
     {
          unsigned mask_num = qubit_index / 32;
          gates_.S[pos_[qubit_index] * gates_.nmasks_8 + mask_num] |=
              (1 << (qubit_index - mask_num * 32));
          ++pos_[qubit_index];
          ++gates_.num_gates;
          if (pos_[qubit_index] >= max_storage_depth) {
               sync();
          }
     }

     void X_(unsigned qubit_index)
     {
          H_(qubit_index);
          S_(qubit_index);
          S_(qubit_index);
          H_(qubit_index);
     }

     void CNOT_(unsigned control_qubit_index, unsigned target_qubit_index)
     {
          auto pos =
              std::max(pos_[control_qubit_index], pos_[target_qubit_index]);
          gates_.C[pos].emplace_back(control_qubit_index, target_qubit_index);
          ++pos;
          pos_[control_qubit_index] = pos;
          pos_[target_qubit_index] = pos;
          ++gates_.num_gates;
          if (pos_[control_qubit_index] >= max_storage_depth) {
               sync();
          }
     }

     std::tuple<bool, unsigned> determine_outcome_type_(unsigned qubit_index)
     {
          auto p(num_qubits_);
          for (; p < 2 * num_qubits_ && !get_stab_x_(p, qubit_index); ++p)
               ;
          const auto random_outcome(p != 2 * num_qubits_);

          return std::make_tuple(random_outcome, random_outcome ? p : 0);
     }

     void pre_measurement_random_(unsigned qubit_index, unsigned p)
     {
#pragma omp parallel for schedule(static, 32)
          for (auto j = 0U; j < 2 * num_qubits_; ++j) {
               if (j != p && get_stab_x_(j, qubit_index)) {
                    rowsum_(j, p);
               }
          }

          // set row p-num_qubits equal to row p
          // set row p equal to 0 except z_{p,qubit} = 1 and r_p 0 or 1 with
          // equal probability
          const auto start_p_m_num_qubits(get_index_(p - num_qubits_, 0));
          const auto start_p(get_index_(p, 0));
          for (auto k(0U); k < row_length_; ++k) {
               x_[start_p_m_num_qubits + k] = x_[start_p + k];
               x_[start_p + k] = 0;
               z_[start_p_m_num_qubits + k] = z_[start_p + k];
               z_[start_p + k] = 0;
          }
          set_stab_z_(p, qubit_index, 1);
          set_phase_bit_(p - num_qubits_, get_phase_bit_(p));
     }

     void pre_measurement_deterministic_(unsigned qubit_index)
     {
          const auto start(get_index_(2 * num_qubits_, 0));

          // set row 2*num_qubits equal to 0 (this is a previously unused row)
          std::fill(begin(x_) + start, begin(x_) + start + row_length_, 0);
          std::fill(begin(z_) + start, begin(z_) + start + row_length_, 0);
          set_phase_bit_(2 * num_qubits_, 0);

          // rowsum(2*num_qubits, i + num_qubits) for all i in
          // 0,...,num_qubits-1 (i.e. a destabilizer)
          // for which x_{i,qubit} = 1
          for (auto l(0U); l < num_qubits_; ++l) {
               if (get_stab_x_(l, qubit_index)) {
                    rowsum_(2 * num_qubits_, l + num_qubits_);
               }
          }
     }

     bool measure_(unsigned qubit_index)
     {
#if __cplusplus >= 201703L
          const auto [random_outcome, p] = determine_outcome_type_(qubit_index);
#else
          auto random_outcome(false);
          auto p(0U);
          std::tie(random_outcome, p) = determine_outcome_type_(qubit_index);
#endif
          if (random_outcome) {  // Case I (random outcome)
               pre_measurement_random_(qubit_index, p);
               bool measurement(dist_(rnd_eng_));
               set_phase_bit_(p, measurement);
               return measurement;
          }
          else {  // Case II (deterministic outcome)
               pre_measurement_deterministic_(qubit_index);
               return get_phase_bit_(2 * num_qubits_);
          }
     }

     double get_probability_recursive_(
         std::vector<bool>::const_iterator bitstring_begin,
         std::vector<bool>::const_iterator bitstring_end,
         std::vector<unsigned>::const_iterator qubit_ids_begin,
         std::vector<unsigned>::const_iterator qubit_ids_end)
     {
          const auto& state(*(bitstring_begin++));
          const auto& qubit_id(*(qubit_ids_begin++));

          QUBIT_ALLOCATE_CHECK(qubit_id, "get_probability");
          const auto& qubit_index = allocated_qubits_[qubit_id];

#if __cplusplus >= 201703L
          const auto [random_outcome, p] = determine_outcome_type_(qubit_index);
#else
          auto random_outcome(false);
          auto p(0U);
          std::tie(random_outcome, p) = determine_outcome_type_(qubit_index);
#endif
          if (random_outcome) {  // Case I (random outcome)
               if (bitstring_begin == bitstring_end) {
                    return 0.5;
               }
               else {
                    const auto r_bak = r_;
                    const auto x_bak = x_;
                    const auto z_bak = z_;

                    pre_measurement_random_(qubit_index, p);

                    /* Only need to consider the case where `state` was measured
                     * as the other case would anyway not contribute to the
                     * resulting probability.
                     */
                    set_phase_bit_(p, state);
                    const auto prob(0.5 * get_probability_recursive_(
                                              bitstring_begin, bitstring_end,
                                              qubit_ids_begin, qubit_ids_end));
                    r_ = r_bak;
                    x_ = x_bak;
                    z_ = z_bak;
                    return prob;
               }
          }
          else {  // Case II (deterministic outcome)
               const auto r_bak(get_phase_bit_(2 * num_qubits_));
               const auto start(get_index_(2 * num_qubits_, 0));
               const AlignedIntVec x_bak(begin(x_) + start,
                                         begin(x_) + start + row_length_);
               const AlignedIntVec z_bak(begin(z_) + start,
                                         begin(z_) + start + row_length_);

               pre_measurement_deterministic_(qubit_index);

               double prob(get_phase_bit_(2 * num_qubits_) == state);

               /*
                * Essentially cover three cases:
                *  - if bitstring_begin == bitstring_end: do nothing
                *  - if prob == 0.0: do nothing
                *  - if prob == 1.0 and bitstring_begin != bitstring_end:
                *      -> calculate further probabilities
                */
               if (bitstring_begin != bitstring_end && prob != 0.) {
                    prob = get_probability_recursive_(
                        bitstring_begin, bitstring_end, qubit_ids_begin,
                        qubit_ids_end);
               }

               std::copy(begin(x_bak), end(x_bak), begin(x_) + start);
               std::copy(begin(z_bak), end(z_bak), begin(z_) + start);
               set_phase_bit_(2 * num_qubits_, r_bak);
               return prob;
          }
     }

     void collapse_wavefunction_(unsigned qubit_index, bool state)
     {
#if __cplusplus >= 201703L
          const auto [random_outcome, p] = determine_outcome_type_(qubit_index);
#else
          auto random_outcome(false);
          auto p(0U);
          std::tie(random_outcome, p) = determine_outcome_type_(qubit_index);
#endif
          if (random_outcome) {  // Case I (random outcome)
               pre_measurement_random_(qubit_index, p);
               set_phase_bit_(p, state);
          }
          else {  // Case II (deterministic outcome)
               pre_measurement_deterministic_(qubit_index);
               if (get_phase_bit_(2 * num_qubits_) != state) {
                    THROW_RUNTIME_ERROR(
                        "collapse_wavefunction(): "
                        "Invalid collapse! Probability is ~0.");
               }
          }
     }

     bool set_qubits_(unsigned qubit_index, bool state)
     {
#if __cplusplus >= 201703L
          const auto [random_outcome, p] = determine_outcome_type_(qubit_index);
#else
          auto random_outcome(false);
          auto p(0U);
          std::tie(random_outcome, p) = determine_outcome_type_(qubit_index);
#endif
          if (random_outcome) {  // Case I (random outcome)
               pre_measurement_random_(qubit_index, p);
               set_phase_bit_(p, state);
          }
          else {  // Case II (deterministic outcome)
               const auto r_bak(get_phase_bit_(2 * num_qubits_));
               const auto start(get_index_(2 * num_qubits_, 0));
               const AlignedIntVec x_bak(begin(x_) + start,
                                         begin(x_) + start + row_length_);
               const AlignedIntVec z_bak(begin(z_) + start,
                                         begin(z_) + start + row_length_);

               pre_measurement_deterministic_(qubit_index);

               if (get_phase_bit_(2 * num_qubits_) != state) {
                    std::copy(begin(x_bak), end(x_bak), begin(x_) + start);
                    std::copy(begin(z_bak), end(z_bak), begin(z_) + start);
                    set_phase_bit_(2 * num_qubits_, r_bak);
                    return true;
               }
          }
          return false;
     }

     const unsigned num_qubits_;
     const unsigned row_length_;

     AlignedIntVec x_;  // len is N x 2N+1
     AlignedIntVec z_;  // len is N x 2N+1
     AlignedIntVec r_;  // len is 2N+1

     std::mt19937 rnd_eng_;
     std::uniform_int_distribution<int> dist_;

     Gates gates_;
     std::vector<unsigned> pos_;

     // Qubits
     Map allocated_qubits_;
     Vec available_qubits_;
};

std::ostream& operator<<(std::ostream& out, StabilizerSimulator const& sim)
{
     for (auto stab(0U); stab < 2 * sim.num_qubits_ + 1; ++stab) {
          for (unsigned qubit = 0; qubit < sim.num_qubits_; ++qubit) {
               out << sim.get_stab_x_(stab, qubit) << " ";
          }
          for (unsigned qubit = 0; qubit < sim.num_qubits_; ++qubit) {
               out << sim.get_stab_z_(stab, qubit) << " ";
          }
          out << "| " << sim.get_phase_bit_(stab) << "\n";
     }
     return out;
}
