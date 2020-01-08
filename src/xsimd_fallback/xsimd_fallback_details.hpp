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

/*! \file */

#pragma once

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

#ifdef __INTEL_COMPILER
#     define PRAGMA_LOOP_IVDEP _Pragma("ivdep")
#elif defined(__clang__)
#     define PRAGMA_LOOP_IVDEP
#elif defined(__GNUC__)
#     define PRAGMA_LOOP_IVDEP _Pragma("GCC ivdep")
#elif defined(_MSC_VER)
#     define PRAGMA_LOOP_IVDEP __pragma("loop ivdep")
#else
#     define PRAGMA_LOOP_IVDEP
#endif  // __INTEL_COMPILER || __GNUC__ || _MSC_VER

#include "xsimd_fallback_instruction_set.hpp"

#define XSIMD_FALLBACK_DEFAULT_SIZE 8

#if XSIMD_FALLBACK_X86_INSTR_SET >= XSIMD_FALLBACK_X86_AVX512_VERSION
#     define XSIMD_FALLBACK_BATCH_INT8_SIZE 64
#     define XSIMD_FALLBACK_BATCH_INT16_SIZE 32
#     define XSIMD_FALLBACK_BATCH_INT32_SIZE 16
#     define XSIMD_FALLBACK_BATCH_INT64_SIZE 8
#     define XSIMD_FALLBACK_BATCH_FLOAT_SIZE 16
#     define XSIMD_FALLBACK_BATCH_DOUBLE_SIZE 8
#elif XSIMD_FALLBACK_X86_INSTR_SET >= XSIMD_FALLBACK_X86_AVX_VERSION
#     define XSIMD_FALLBACK_BATCH_INT8_SIZE 32
#     define XSIMD_FALLBACK_BATCH_INT16_SIZE 16
#     define XSIMD_FALLBACK_BATCH_INT32_SIZE 8
#     define XSIMD_FALLBACK_BATCH_INT64_SIZE 4
#     define XSIMD_FALLBACK_BATCH_FLOAT_SIZE 8
#     define XSIMD_FALLBACK_BATCH_DOUBLE_SIZE 4
#elif XSIMD_FALLBACK_X86_INSTR_SET >= XSIMD_FALLBACK_X86_SSE2_VERSION
#     define XSIMD_FALLBACK_BATCH_INT8_SIZE 16
#     define XSIMD_FALLBACK_BATCH_INT16_SIZE 8
#     define XSIMD_FALLBACK_BATCH_INT32_SIZE 4
#     define XSIMD_FALLBACK_BATCH_INT64_SIZE 2
#     define XSIMD_FALLBACK_BATCH_FLOAT_SIZE 4
#     define XSIMD_FALLBACK_BATCH_DOUBLE_SIZE 2
#elif XSIMD_FALLBACK_ARM_INSTR_SET >= XSIMD_FALLBACK_ARM7_NEON_VERSION
#     define XSIMD_FALLBACK_BATCH_INT8_SIZE 16
#     define XSIMD_FALLBACK_BATCH_INT16_SIZE 8
#     define XSIMD_FALLBACK_BATCH_INT32_SIZE 4
#     define XSIMD_FALLBACK_BATCH_INT64_SIZE 2
#     define XSIMD_FALLBACK_BATCH_FLOAT_SIZE 4
#endif

#if XSIMD_FALLBACK_ARM_INSTR_SET >= XSIMD_FALLBACK_ARM8_64_NEON_VERSION
#     define XSIMD_FALLBACK_BATCH_DOUBLE_SIZE 2
#endif

#ifndef XSIMD_DEFAULT_ALIGNMENT
#     if XSIMD_FALLBACK_X86_INSTR_SET >= XSIMD_FALLBACK_X86_AVX512_VERSION
#          define XSIMD_DEFAULT_ALIGNMENT 64
#     elif XSIMD_FALLBACK_X86_INSTR_SET >= XSIMD_FALLBACK_X86_AVX_VERSION
#          define XSIMD_DEFAULT_ALIGNMENT 32
#     elif XSIMD_FALLBACK_X86_INSTR_SET >= XSIMD_FALLBACK_X86_SSE2_VERSION
#          define XSIMD_DEFAULT_ALIGNMENT 16
#     elif XSIMD_FALLBACK_ARM_INSTR_SET >= XSIMD_FALLBACK_ARM8_64_NEON_VERSION
#          define XSIMD_DEFAULT_ALIGNMENT 32
#     elif XSIMD_FALLBACK_ARM_INSTR_SET >= XSIMD_FALLBACK_ARM7_NEON_VERSION
#          define XSIMD_DEFAULT_ALIGNMENT 16
#     endif
#endif  // !XSIMD_DEFAULT_ALIGNMENT

namespace xsimd
{
// Pre-declarations
template <class X>
class simd_base;
template <typename T, std::size_t N>
class batch;
template <typename T>
class cst_expr_impl;
template <typename T, std::size_t N>
class batch_expr_impl;
template <typename T, typename tag_type>
class unary_op_expr_impl;
template <typename T, typename U, typename tag_type>
class binary_op_expr_impl;
template <typename T>
struct expr_base;

//! Namespace containing helper classes and functions
namespace details
{
     //! Convenience template alias
     template <typename T>
     using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

     // =============================================================================
     // Type traits classes

     //! Type trait class to detect batch<T, N> values
     /*!
      * \class is_batch
      * General implementation for all types
      */
     template <typename T>
     struct is_batch : std::false_type
     {};
     //! Type trait class to detect batch<T, N> values
     /*!
      * \class is_batch
      * Specialization for batch<T, N>
      */
     template <typename T, std::size_t N>
     struct is_batch<batch<T, N>> : std::true_type
     {};

     //! Type trait class to detect expr_base<T> values
     /*!
      * \class is_expr
      * General implementation for all types
      */
     template <typename T>
     struct is_expr : std::false_type
     {};
     //! Type trait class to detect expr_base<T> values
     /*!
      * \class is_expr
      * Specialization for expr_base<T>
      */
     template <typename T>
     struct is_expr<expr_base<T>> : std::true_type
     {};

     //! Type trait class to detect valid expressions
     /*!
      * \class is_valid_expr
      * A valid expression for our purposes is either:
      *   - an arithmetic value
      *   - a batch<T, N> value (provided that it does not inherit from
      *     simd_base since those are from the XSIMD library)
      *   - a class inheriting from expr_base
      */
     template <typename T, typename U = remove_cvref_t<T>>
     struct is_valid_expr
         : std::conditional_t<
               std::is_arithmetic<U>::value
                   || (is_batch<U>::value
                       && !std::is_convertible<U*, simd_base<U>*>::value)
                   || std::is_convertible<U*, expr_base<U>*>::value,
               std::true_type, std::false_type>
     {};

     // =============================================================================

     // Taken from XSIMD
     template <typename T, std::size_t... Is>
     constexpr auto array_from_scalar_impl(const T& scalar,
                                           std::index_sequence<Is...>)
     {
          // You can safely ignore this silly ternary, the "scalar" is all
          // that matters. The rest is just a dirty workaround...
          return std::array<T, sizeof...(Is)>{(Is + 1) ? scalar : T()...};
     }

     // Taken from XSIMD
     template <typename T, std::size_t N>
     constexpr std::array<T, N> array_from_scalar(const T& scalar)
     {
          return array_from_scalar_impl(scalar, std::make_index_sequence<N>());
     }

     // Taken from XSIMD
     template <typename T, std::size_t... Is>
     constexpr std::array<T, sizeof...(Is)> array_from_pointer_impl(
         const T* c_array, std::index_sequence<Is...>)
     {
          return std::array<T, sizeof...(Is)>{c_array[Is]...};
     }

     // Taken from XSIMD
     template <typename T, std::size_t N>
     constexpr std::array<T, N> array_from_pointer(const T* c_array)
     {
          return array_from_pointer_impl(c_array,
                                         std::make_index_sequence<N>());
     }

     // =============================================================================
     // Expression size calculations

     //! Trait class to calculate the size of an expression
     //! \class expr_size_impl
     template <typename T>
     struct expr_size_impl
     {
          static_assert(std::is_arithmetic<T>::value,
                        "T needs to be arithmetic!");
          static constexpr auto size = std::numeric_limits<std::size_t>::max();
     };

     template <typename T>
     struct expr_size_impl<cst_expr_impl<T>>
     {
          static constexpr auto size = std::numeric_limits<std::size_t>::max();
     };

     template <typename T, std::size_t N>
     struct expr_size_impl<batch_expr_impl<T, N>>
     {
          static constexpr auto size = N;
     };

     template <typename T, std::size_t N>
     struct expr_size_impl<batch<T, N>>
     {
          static constexpr auto size = N;
     };

     template <typename T, typename tag_type>
     struct expr_size_impl<unary_op_expr_impl<T, tag_type>>
     {
          static constexpr auto size = expr_size_impl<T>::size;
     };

     template <typename T, typename U, typename tag_type>
     struct expr_size_impl<binary_op_expr_impl<T, U, tag_type>>
     {
          static constexpr auto size
              = std::min(expr_size_impl<T>::size, expr_size_impl<U>::size);
     };

     template <typename T>
     using expr_size = expr_size_impl<remove_cvref_t<T>>;

     // =============================================================================
     // Expression creation helper function

     //! Expression creation helper class
     /*!
      * \class expr_maker
      * Implementation for general expressions
      */
     template <typename T, bool = std::is_arithmetic<T>::value>
     struct expr_maker
     {
          template <typename U>
          static constexpr auto make(U&& u)
          {
               using real_type = typename expr_base<const U&>::type;
               return static_cast<real_type>(u);
          }
     };
     //! Expression creation helper class
     /*!
      * Specialisation for arithmetic types
      */
     template <typename T>
     struct expr_maker<T, true>
     {
          static constexpr auto make(T expr)
          {
               return cst_expr_impl<T>(expr);
          }
     };
     //! Expression creation helper class
     /*!
      * Specialisation for batch<T, N> values
      */
     template <typename T, std::size_t N>
     struct expr_maker<batch<T, N>, false>
     {
          static constexpr auto make(const batch<T, N>& expr)
          {
               return batch_expr_impl<T, N>(expr);
          }
     };

     //! Convenience function to generate expressions from an arbitrary value
     /*!
      * \tparam T Some type
      * \param t Some object
      * \return An expression corresponding to the input parameter
      */
     template <typename T>
     constexpr auto make_expr(T&& t)
     {
          return expr_maker<remove_cvref_t<T>>::make(std::forward<T>(t));
     }

}  // namespace details
}  // namespace xsimd

// =============================================================================
