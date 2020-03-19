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

#include "details/helper_functions.hpp"
#include "details/popcnt32.hpp"
#include "xsimd_include.hpp"

/* If for some weird reason throwing std::runtime_error leads to segmentation
 * faults (e.g. pybind11 2.2.4 & clang 10.0 on Mac OS X) use the corresponding
 * CMake option to enable the workaround.
 */
#ifdef PYBIND11_NO_THROW_RUNTIME_ERROR
#     define THROW_RUNTIME_ERROR(msg)                                          \
          PyErr_SetString(PyExc_RuntimeError, msg);                            \
          throw pybind11::error_already_set()
#else
#     define THROW_RUNTIME_ERROR(msg) throw std::runtime_error(msg)
#endif /* PYBIND11_THROW_RUNTIME_ERROR */

#define QUBIT_ALLOCATE_CHECK(qubit_id, gate_name)                              \
     if (allocated_qubits_.count(qubit_id) == 0) {                             \
          THROW_RUNTIME_ERROR("Error (" gate_name                              \
                              "): Qubit ID is not "                            \
                              "valid! (ie. unallocated Qubit)");               \
     }

//! Maximum storage depth for H, S and CNOT gates
constexpr auto max_storage_depth = 10U;

//! Aligned vector of unsigned integers
using AlignedIntVec
    = std::vector<uint32_t, xsimd::aligned_allocator<uint32_t, 256>>;
//! Vector of CNOT operations
using VecOfCNOT = std::vector<std::vector<std::pair<unsigned, unsigned>>>;

namespace details {
//! Gate storage
/*!
 * Class used to cache gate operations
 */
struct Gates
{
public:
     //! Constructor
     /*!
      * \param n number of qubits
      * \param max_d_a Maximum number of gates to cache
      */
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

     //! Reset the state of the cache
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
}  // details

#ifdef _STABSIM_TEST_ENV
#  define private public
#endif // _STABSIM_TEST_ENV

/*!
 * \brief Stabilizer simulator class
 *
 * Simulator class implemented using the stabilizer formalism.
 */
class StabilizerSimulator
{
     using calc_type = double;
     using Vec = std::vector<unsigned>;
     using Map = std::map<unsigned, unsigned>;

public:
     //! Constructor
     /*!
      * \param num_qubits Maximum number of allocated qubits
      * \param seed Seed for pseudo-random number generator
      */
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

     //! Allocate one qubit with a given id
     /*!
      * \param qubit_id ID of the qubit to allocate
      * \throw std::runtime_error if the qubit de-allocation fails either due
      *        to the qubit being already allocated or if the maximum number of
      *        allocated qubit has been reached.
      */
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

     //! Convenience function to allocate all qubits
     void allocate_all_qubits()
     {
          for (auto id(0UL); id < num_qubits_; ++id) {
               allocate_qubit(id);
          }
     }

     //! De-allocate one qubit with a given id
     /*!
      * \param qubit_id ID of the qubit to de-allocate
      * \throw std::runtime_error if the qubit de-allocation fails either due
      *        to the qubit not being allocated or not in a classical state.
      */
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

     //! Apply a Hadamard gate
     /*!
      * \param qubit_id ID of a qubit
      * \throw std::runtime_error if the desired qubit is not already allocated
      */
     void H(unsigned qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "HGate");
          H_(allocated_qubits_[qubit_id]);
     }

     //! Apply an S gate
     /*!
      * \param qubit_id ID of a qubit
      * \throw std::runtime_error if the desired qubit is not already allocated
      */
     void S(unsigned qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "SGate");
          S_(allocated_qubits_[qubit_id]);
     }

     //! Apply a X (or NOT) gate
     /*!
      * \param qubit_id ID of a qubit
      * \throw std::runtime_error if the desired qubit is not already allocated
      */
     void X(unsigned qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(qubit_id, "XGate");
          X_(allocated_qubits_[qubit_id]);
     }

     //! Apply a controlled-NOT gate
     /*!
      * \param control_qubit_id ID of the control qubit
      * \param target_qubit_id ID of the target qubit
      * \throw std::runtime_error if the desired qubit is not already allocated
      */
     void CNOT(unsigned control_qubit_id, unsigned target_qubit_id)
     {
          QUBIT_ALLOCATE_CHECK(control_qubit_id, "CNOTGate");
          QUBIT_ALLOCATE_CHECK(target_qubit_id, "CNOTGate");
          CNOT_(allocated_qubits_[control_qubit_id],
                allocated_qubits_[target_qubit_id]);
     }

     // =========================================================================
     // Misc. ProjectQ utility functions

     //! Apply measurement gates to a set of qubits
     /*!
      * \param qubit_ids Array of qubit IDs
      * \return Array of the measured values
      * \throw std::runtime_error if any of the qubit IDs are invalid
      */
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

     //! Apply a measurement gate to a single qubit
     /*!
      * \param qubit_id ID of a qubit
      * \return Measured value
      * \throw std::runtime_error if the desired qubit is not already allocated
      */
     bool measure(unsigned qubit_id)
     {
          sync();
          QUBIT_ALLOCATE_CHECK(qubit_id, "MeasureGate");
          return measure_(allocated_qubits_[qubit_id]);
     }

     //! Get the classical value of a qubit
     /*!
      * \param qubit_id ID of a qubit
      * \param tol Tolerance for floating point comparison
      * \return Classical value of the qubit
      * \throw std::runtime_error if the desired qubit is not already allocated
      */
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

     //! Test whether a qubit is in a classical state or not
     /*!
      * \param qubit_id ID of a qubit
      * \param tol Tolerance for floating point comparison
      * \return \c true/\c false depending on whether the qubit is in a 
      *         classical state or not
      * \throw std::runtime_error if the desired qubit is not already allocated
      */
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

     /*!
      * \brief Return the probability of the outcome bit_string when measuring 
      *        the quantum register qureg.
      * \param bitstring Measurement outcome.
      * \param qubit_ids Array of qubit IDs
      * \return The required probability
      * \throw std::runtime_error if any of the desired qubits are not already
      *        allocated
      */
     double get_probability(const std::vector<bool>& bitstring,
                            const std::vector<unsigned>& qubit_ids)
     {
          sync();
          if (bitstring.empty()) {
               return 1.0;
          }
          assert(bitstring.size() == qubit_ids.size());
          return get_probability_recursive_(cbegin(bitstring), cend(bitstring),
                                            cbegin(qubit_ids), cend(qubit_ids));
     }

     //! Collapse a quantum register onto a classical basis state.
     /*!
      * \param qubit_ids Array of qubit IDs
      * \param bitstring Measurement outcome for each of the qubits
      * \throw std::runtime_error if any of the desired qubits are not already
      *        allocated
      */
     void collapse_wavefunction(const std::vector<unsigned>& qubit_ids,
                                const std::vector<bool>& bitstring)
     {
          sync();
          if (bitstring.empty()) {
               return;
          }
          assert(qubit_ids.size() == bitstring.size());
          auto qubit_it(cbegin(qubit_ids));
          auto bitstring_it(cbegin(bitstring));
          const auto qubit_end(cend(qubit_ids));

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
          catch (std::exception& e) {
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
          auto qubit_it(cbegin(qubit_ids));
          auto bitstring_it(cbegin(bitstring));
          const auto qubit_end(cend(qubit_ids));
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

     //! Flush gate cache
     void sync()
     {
          if (gates_.num_gates == 0) {
               return;
          }

#pragma omp parallel for schedule(static, 32)
          for (auto stab = 0U; stab < 2 * num_qubits_; ++stab) {
               const auto& nmasks_8 = gates_.nmasks_8;
               for (auto d = 0U; d < gates_.max_d; ++d) {
                    h_and_s_kernel_(cbegin(gates_.H) + d * nmasks_8,
                                    cbegin(gates_.H) + (d + 1) * nmasks_8,
                                    cbegin(gates_.S) + d * nmasks_8,
                                    cbegin(gates_.S) + (d + 1) * nmasks_8,
                                    stab);
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

     // Execute one iteration of the compute_phase function
     template <typename T, typename U>
     inline constexpr void compute_phase_iter_(const T& xh, const T& xi,
                                               const T& zh, const T& zi,
                                               U& exp_high, U& exp_low) const
     {
          /*
                uint32_t q = (stab_i_x | stab_h_z) & (stab_i_z | ~stab_h_x) &
                             (~stab_i_z | stab_h_x);
                uint32_t r = (stab_i_x | stab_i_z) & (stab_h_x | stab_h_z);
                uint32_t s = ~(~(stab_i_x ^ stab_h_x) & ~(stab_i_z ^ stab_h_z));
                uint32_t r_and_s = r & s;
                exp_high = (exp_low & r_and_s) ^ (exp_high ^ (q & r_and_s));
                exp_low ^= r_and_s;
          */
          auto r_and_s = ((xi | zi) & (xh | zh)) & ((xi ^ xh) | (zi ^ zh));
          exp_high = (exp_low & r_and_s)
                     ^ (exp_high
                        ^ (((xi | zh) & (~xh | zi) & (~zi | xh)) & r_and_s));
          //               |<--------------- q --------------->|
          exp_low = exp_low ^ r_and_s;
     }

     unsigned compute_phase_(unsigned stabilizer_h, unsigned stabilizer_i) const
     {
          using value_type = std::decay_t<decltype(x_[0])>;
          using traits = xsimd::simd_traits<value_type>;
          using batch_type = typename traits::type;
          using batch_const_type =
              typename xsimd::const_simd_traits<value_type>::type;
          constexpr auto simd_size = traits::size;

          auto size = row_length_;
          const auto start_index_h = stabilizer_h * row_length_;
          const auto start_index_i = stabilizer_i * row_length_;

          auto xh_it = cbegin(x_) + start_index_h;
          auto xi_it = cbegin(x_) + start_index_i;
          auto zh_it = cbegin(z_) + start_index_h;
          auto zi_it = cbegin(z_) + start_index_i;

          const auto align_begin_1
              = xsimd::get_alignment_offset(&(*xh_it), size, simd_size);
          const auto align_begin_2
              = xsimd::get_alignment_offset(&(*zh_it), size, simd_size);
          const auto align_end
              = align_begin_1 + ((size - align_begin_1) & ~(simd_size - 1));

          assert(align_begin_1 == align_begin_2);
          assert(align_begin_1 == 0);

          value_type exp_high(0), exp_low(0);

          batch_const_type xh_batch, xi_batch, zh_batch, zi_batch;
          batch_type exp_high_batch(value_type(0)),
              exp_low_batch(value_type(0));
          PRAGMA_LOOP_IVDEP
          for (auto i = align_begin_1; i < align_end; i += simd_size) {
               xsimd::load_aligned(&xh_it[i], xh_batch);
               xsimd::load_aligned(&xi_it[i], xi_batch);
               xsimd::load_aligned(&zh_it[i], zh_batch);
               xsimd::load_aligned(&zi_it[i], zi_batch);

               compute_phase_iter_(xh_batch, xi_batch, zh_batch, zi_batch,
                                   exp_high_batch, exp_low_batch);
          }

          // reduce exp_high and exp_low
          alignas(32) std::array<uint32_t, 8> tmp;
          xsimd::store_aligned(tmp.data(), exp_high_batch);
          details::apply_array_func<std::bit_xor<value_type>>(tmp, exp_high);

          xsimd::store_aligned(tmp.data(), exp_low_batch);
          details::apply_array_func<std::bit_xor<value_type>>(tmp, exp_low);

          PRAGMA_LOOP_IVDEP
          for (std::size_t i = align_end; i < size; ++i) {
               compute_phase_iter_(xh_it[i], xi_it[i], zh_it[i], zi_it[i],
                                   exp_high, exp_low);
          }

          const auto exponent = 2 * popcnt32(exp_high) + popcnt32(exp_low);

          // compute positive modulo.
          return (((exponent + 2 * get_phase_bit_(stabilizer_h)
                    + 2 * get_phase_bit_(stabilizer_i))
                   % 4)
                  + 4)
                 % 4;
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

          //      // x_hj ^= x_ij
          //      // x[start_h + i] ^= x[start_i + i];
          xsimd::self_transform(
              begin(x_) + start_h, begin(x_) + start_h + row_length_,
              begin(x_) + start_i,
              [](const auto& xh, const auto& xi) { return xh ^ xi; });

          //      // same for z_hj:
          //      // z[start_h + i] ^= z[start_i + i];
          xsimd::self_transform(
              begin(z_) + start_h, begin(z_) + start_h + row_length_,
              begin(z_) + start_i,
              [](const auto& zh, const auto& zi) { return zh ^ zi; });
     }

     // Execute one iteration of the H and S kernel
     template <typename T, typename U, typename V, typename W, typename X>
     inline constexpr void h_and_s_kernel_iter_(T& x, U& z, const V& H,
                                                const W& S, X& phase_sum)
     {
          auto smasked_x = x & S;
          phase_sum = phase_sum ^ (smasked_x & z);
          z = z ^ smasked_x;

          phase_sum = phase_sum ^ (H & x & z);
          x = x ^ z;
          z = z ^ (x & H);
          x = x ^ z;
     }

     // Execute the H and S kernel
     inline void h_and_s_kernel_(AlignedIntVec::const_iterator Hmasks_begin,
                                 AlignedIntVec::const_iterator Hmasks_end,
                                 AlignedIntVec::const_iterator Smasks_begin,
                                 AlignedIntVec::const_iterator Smasks_end,
                                 unsigned stab)
     {
          using value_type = std::decay_t<decltype(x_[0])>;
          using traits = xsimd::simd_traits<value_type>;
          using batch_type = typename traits::type;
          using batch_const_type =
              typename xsimd::const_simd_traits<value_type>::type;
          constexpr auto simd_size = traits::size;

          auto size = static_cast<std::size_t>(
              std::distance(Hmasks_begin, Hmasks_end));

          auto x_it = begin(x_) + row_length_ * stab;
          auto z_it = begin(z_) + row_length_ * stab;
          auto H_it(Hmasks_begin);
          auto S_it(Smasks_begin);

          const auto align_begin_1
              = xsimd::get_alignment_offset(&(*x_it), size, simd_size);
          const auto align_begin_2
              = xsimd::get_alignment_offset(&(*z_it), size, simd_size);
          const auto align_begin_3
              = xsimd::get_alignment_offset(&(*H_it), size, simd_size);
          const auto align_begin_4
              = xsimd::get_alignment_offset(&(*S_it), size, simd_size);
          const auto align_end
              = align_begin_1 + ((size - align_begin_1) & ~(simd_size - 1));

          assert(align_begin_1 == align_begin_2
                 && align_begin_2 == align_begin_3
                 && align_begin_3 == align_begin_4);
          assert(align_begin_1 == 0);

          value_type phase_sum(0);

          batch_const_type H_batch, S_batch;
          batch_type x_batch, z_batch, phase_sum_batch(value_type(0));
          PRAGMA_LOOP_IVDEP
          for (auto i = align_begin_1; i < align_end; i += simd_size) {
               xsimd::load_aligned(&x_it[i], x_batch);
               xsimd::load_aligned(&z_it[i], z_batch);
               xsimd::load_aligned(&H_it[i], H_batch);
               xsimd::load_aligned(&S_it[i], S_batch);

               h_and_s_kernel_iter_(x_batch, z_batch, H_batch, S_batch,
                                    phase_sum_batch);

               xsimd::store_aligned(&x_it[i], x_batch);
               xsimd::store_aligned(&z_it[i], z_batch);
          }

          // phase_sum calculation
          alignas(32) std::array<uint32_t, 8> tmp;
          xsimd::store_aligned(tmp.data(), phase_sum_batch);
          details::apply_array_func<std::bit_xor<value_type>>(tmp, phase_sum);

          PRAGMA_LOOP_IVDEP
          for (std::size_t i = align_end; i < size; ++i) {
               h_and_s_kernel_iter_(x_it[i], z_it[i], H_it[i], S_it[i],
                                    phase_sum);
          }

          set_phase_bit_(stab,
                         get_phase_bit_(stab) ^ (popcnt32(phase_sum) & 1));
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
          gates_.H[pos_[qubit_index] * gates_.nmasks_8 + mask_num]
              |= (1 << (qubit_index - mask_num * 32));
          ++pos_[qubit_index];
          ++gates_.num_gates;
          if (pos_[qubit_index] >= max_storage_depth) {
               sync();
          }
     }

     void S_(unsigned qubit_index)
     {
          unsigned mask_num = qubit_index / 32;
          gates_.S[pos_[qubit_index] * gates_.nmasks_8 + mask_num]
              |= (1 << (qubit_index - mask_num * 32));
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
          auto pos
              = std::max(pos_[control_qubit_index], pos_[target_qubit_index]);
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
                    const auto prob(0.5
                                    * get_probability_recursive_(
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

     details::Gates gates_;
     std::vector<unsigned> pos_;

     // Qubits
     Map allocated_qubits_;
     Vec available_qubits_;
};


#ifdef _STABSIM_TEST_ENV
#  undef private
#endif // _STABSIM_TEST_ENV

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
