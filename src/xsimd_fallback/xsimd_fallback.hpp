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

#include <array>
#include <cassert>
#include <iterator>
#include <tuple>

#include "xsimd_fallback_details.hpp"

namespace xsimd
{
// Pre-declarations
template <typename T, std::size_t N>
class batch;

//! Namespace containing all known tags
namespace tags
{
     //! Tag for constant values
     struct cst_tag
     {};
     //! Tag for batch values
     struct batch_tag
     {};
     //! Tag for the "logical not" operation
     struct logical_not_tag
     {};
     //! Tag for the "logical and" operation
     struct logical_and_tag
     {};
     //! Tag for the "logical or" operation
     struct logical_or_tag
     {};
     //! Tag for the "logical xor" operation
     struct logical_xor_tag
     {};
     //! Tag for the "negate" operation
     struct negate_tag
     {};
     //! Tag for the "add" operation
     struct add_tag
     {};
     //! Tag for the "subtraction" operation
     struct sub_tag
     {};
     //! Tag for the "multiplication" operation
     struct mul_tag
     {};
     //! Tag for the "division" operation
     struct div_tag
     {};
}  // namespace tags

// =============================================================================
// Helper functions to build expressions

/*!
 * \class expr_base
 * \brief Base class for all expressions
 *
 * \tparam derived_t CRTP derived type
 */
template <typename derived_t>
struct expr_base
{
     using type = derived_t;
};

/*!
 * \class cst_expr_impl
 * \brief Expression class encapsulating an arithmetic value
 *
 * \tparam T Underlying arithmetic type
 */
template <typename T>
class cst_expr_impl : public expr_base<cst_expr_impl<T>>
{
public:
     using base_type = expr_base<cst_expr_impl<T>>;

     //! Constructor
     explicit constexpr cst_expr_impl(T val) : val_(val)
     {}

     //! Call operator for visitor access
     template <typename visitor_t>
     constexpr auto operator()(visitor_t visitor) const
     {
          return visitor(tags::cst_tag{}, val_);
     }

private:
     T val_;
};

/*!
 * \class batch_expr_impl
 * \brief Expression class encapsulating a batch<T, N> value
 *
 * \tparam T Underlying arithmetic type
 * \tparam N Number of values within a batch
 */
template <typename T, std::size_t N>
class batch_expr_impl : public expr_base<batch_expr_impl<T, N>>
{
public:
     using base_type = expr_base<batch_expr_impl<T, N>>;

     //! Constructor
     explicit constexpr batch_expr_impl(const batch<T, N>& b) : b_(b)
     {}

     //! Call operator for visitor access
     template <typename visitor_t>
     constexpr auto operator()(visitor_t visitor) const
     {
          return visitor(tags::batch_tag{}, b_);
     }

private:
     const batch<T, N>& b_;
};

/*!
 * \class unary_op_expr_impl
 * \brief Expression class encapsulating an unary expression
 *
 * \tparam T Type of the RHS
 * \tparam tag_type Tag class for that expression
 */
template <typename T, typename tag_type>
class unary_op_expr_impl : public expr_base<unary_op_expr_impl<T, tag_type>>
{
public:
     using base_type = expr_base<unary_op_expr_impl<T, tag_type>>;

     //! Constructor
     explicit constexpr unary_op_expr_impl(T rhs) : rhs_(std::move(rhs))
     {}

     //! Call operator for visitor access
     template <typename visitor_t>
     constexpr auto operator()(visitor_t visitor) const
     {
          return visitor(tag_type{}, rhs_(visitor));
     }

private:
     T rhs_;
};

/*!
 * \class binary_op_expr_impl
 * \brief Expression class encapsulating a binary operators
 *
 * \tparam T Type of the LHS
 * \tparam U Type of the RHS
 * \tparam tag_type Tag class for that expression
 */
template <typename T, typename U, typename tag_type>
class binary_op_expr_impl
    : public expr_base<binary_op_expr_impl<T, U, tag_type>>
{
public:
     using base_type = expr_base<binary_op_expr_impl<T, U, tag_type>>;

     //! Constructor
     constexpr binary_op_expr_impl(T lhs, U rhs)
         : lhs_(std::move(lhs)), rhs_(std::move(rhs))
     {}

     //! Call operator for visitor access
     template <typename visitor_t>
     constexpr auto operator()(visitor_t visitor) const
     {
          return visitor(tag_type{}, lhs_(visitor), rhs_(visitor));
     }

private:
     T lhs_;
     U rhs_;
};

#define DEFINE_UNARY_FUNC(name)                                                \
     template <typename T>                                                     \
     constexpr auto name(T&& lhs)                                              \
     {                                                                         \
          return unary_op_expr_impl<T, tags::name##_tag>(lhs);                 \
     }
#define DEFINE_BINARY_FUNC(name)                                               \
     template <typename T, typename U>                                         \
     constexpr auto name(T&& lhs, U&& rhs)                                     \
     {                                                                         \
          return binary_op_expr_impl<T, U, tags::name##_tag>(lhs, rhs);        \
     }

// Logical tag functions
DEFINE_UNARY_FUNC(logical_not);

DEFINE_BINARY_FUNC(logical_and);
DEFINE_BINARY_FUNC(logical_or);
DEFINE_BINARY_FUNC(logical_xor);

// Arithmetic tag functions
DEFINE_UNARY_FUNC(negate);

DEFINE_BINARY_FUNC(add);
DEFINE_BINARY_FUNC(sub);
DEFINE_BINARY_FUNC(mul);
DEFINE_BINARY_FUNC(div);

#undef DEFINE_UNARY_FUNC
#undef DEFINE_BINARY_FUNC

// =============================================================================

/*!
 * \class eval_visitor
* \brief Expression evaluator class
 *
 * Visitor class to evaluate an expression given an index
 */
struct eval_visitor
{
     //! Constructor
     /*!
      * \param idx Index of value within expression
      */
     explicit constexpr eval_visitor(std::size_t idx) : idx_(idx)
     {}

     //! Call operator for constant values
     template <typename T>
     constexpr auto operator()(tags::cst_tag, T c)
     {
          return c;
     }

     //! Call operator for batch values
     template <typename T>
     constexpr auto operator()(tags::batch_tag, T& b)
     {
          return b[idx_];
     }

     //! Call operator for logical not operations
     template <typename T>
     constexpr auto operator()(tags::logical_not_tag, T rhs)
     {
          return ~rhs;
     }
     //! Call operator for logical and operations
     template <typename T, typename U>
     constexpr auto operator()(tags::logical_and_tag, T lhs, U rhs)
     {
          return lhs & rhs;
     }
     //! Call operator for logical or operations
     template <typename T, typename U>
     constexpr auto operator()(tags::logical_or_tag, T lhs, U rhs)
     {
          return lhs | rhs;
     }
     //! Call operator for logical xor operations
     template <typename T, typename U>
     constexpr auto operator()(tags::logical_xor_tag, T lhs, U rhs)
     {
          return lhs ^ rhs;
     }

     //! Call operator for negation operations
     template <typename T>
     constexpr auto operator()(tags::negate_tag, T rhs)
     {
          return -rhs;
     }

     //! Call operator for addition operations
     template <typename T, typename U>
     constexpr auto operator()(tags::add_tag, T lhs, U rhs)
     {
          return lhs + rhs;
     }

     //! Call operator for subtraction operations
     template <typename T, typename U>
     constexpr auto operator()(tags::sub_tag, T lhs, U rhs)
     {
          return lhs - rhs;
     }

     //! Call operator for multiplication operations
     template <typename T, typename U>
     constexpr auto operator()(tags::mul_tag, T lhs, U rhs)
     {
          return lhs * rhs;
     }

     //! Call operator for division operations
     template <typename T, typename U>
     constexpr auto operator()(tags::div_tag, T lhs, U rhs)
     {
          return lhs / rhs;
     }

private:
     const std::size_t idx_;
};

// =============================================================================
// XSIMD emulation

#define DECLARE_INPLACE_OP(OP)                                                 \
     template <typename U,                                                     \
               typename                                                        \
               = std::enable_if_t<details::is_valid_expr<U>::value, U>>        \
     auto& operator OP(U&& rhs);

/*!
 * \class batch
 * \brief XSIMD-like class representing a batch of values
 *
 * This class simulates the functionality of the class with the same
 * name present within the XSIMD library. It represents a batch of
 * integer or floating point values.
 *
 * One key difference between this class and its XSIMD counterpart is
 * that this implementation does not derive from xsimd::simd_base,
 * which allows some hackery to make it compatible even if XSIMD is
 * used alongside it.
 *
 * \tparam T Underlying arithmetic type
 * \tparam N Number of values within a batch
 *
 * \note This class does not replicate all the functionalities of the
 * batch class in the XSIMD library
 */
template <typename T, std::size_t N>
class batch
{
public:
     friend class batch<const T, N>;
     static_assert(std::is_arithmetic<details::remove_cvref_t<T>>::value,
                   "Template parameter T of batch<T, N> is not arithmetic");

     using value_type = T;
     static constexpr auto size = N;

     using iterator = std::add_pointer_t<value_type>;
     using const_iterator = std::add_pointer_t<std::add_const_t<value_type>>;
     using reverse_iterator = std::reverse_iterator<iterator>;
     using const_reverse_iterator = std::reverse_iterator<const_iterator>;

     explicit batch();
     explicit batch(T data[N]);
     explicit batch(const std::array<T, N>& data);
     explicit batch(T val);
     batch(const batch&) = default;
     batch(batch&&) = default;

     auto operator[](std::size_t idx) const;
     auto& operator[](std::size_t idx);

     batch& operator=(const batch&) = default;
     batch& operator=(batch&&) = default;
     auto& operator=(const batch<const T, N>&);
     auto& operator=(batch<const T, N>&&);
     template <typename U,
               typename = std::enable_if_t<details::is_valid_expr<U>::value, U>>
     auto& operator=(U&& expr);

     DECLARE_INPLACE_OP(+=);
     DECLARE_INPLACE_OP(-=);
     DECLARE_INPLACE_OP(*=);
     DECLARE_INPLACE_OP(/=);

     DECLARE_INPLACE_OP(&=);
     DECLARE_INPLACE_OP(|=);
     DECLARE_INPLACE_OP(^=);

     auto& operator++();
     auto operator++(int);
     auto& operator--();
     auto operator--(int);

     iterator begin();
     iterator end();
     const_iterator begin() const;
     const_iterator end() const;
     const_iterator cbegin() const;
     const_iterator cend() const;

     reverse_iterator rbegin();
     reverse_iterator rend();
     const_reverse_iterator rbegin() const;
     const_reverse_iterator rend() const;
     const_reverse_iterator crbegin() const;
     const_reverse_iterator crend() const;

private:
     std::array<T, N> array_data_;
};

#undef DECLARE_INPLACE_OP

//! Specialisation for batches of constant values
/*!
 * This specialisation is a deviation to the API provided by XSIMD. Its main
 * objective is to allow our fallback library to refer to eternal memory
 * without actually copying the data into the batch itself. This is an extra
 * optimisation for users that do not have XSIMD installed and will not have
 * any effect on valid XSIMD code that is ported to our library.
 *
 * \warning A default initilized batch of constant value has an invalid
 * pointer and should not be used as-is.
 */
template <typename T, std::size_t N>
class batch<const T, N>
{
public:
     static_assert(
         std::is_arithmetic<details::remove_cvref_t<T>>::value,
         "Template parameter T of batch<const T, N> is not arithmetic");

     using value_type = T;
     static constexpr auto size = N;

     using iterator = std::add_pointer_t<std::add_const_t<value_type>>;
     using const_iterator = iterator;
     using reverse_iterator = std::reverse_iterator<const_iterator>;
     using const_reverse_iterator = reverse_iterator;

     constexpr batch();
     explicit constexpr batch(const T data[N]);
     explicit constexpr batch(const std::array<T, N>& data);
     constexpr batch(const batch& rhs);
     constexpr batch(batch&& rhs);

     constexpr auto& operator=(const batch& rhs);
     constexpr auto& operator=(batch&& rhs);

     constexpr auto operator[](std::size_t idx) const;

     constexpr const_iterator begin() const;
     constexpr const_iterator end() const;
     constexpr const_iterator cbegin() const;
     constexpr const_iterator cend() const;

     constexpr const_reverse_iterator rbegin() const;
     constexpr const_reverse_iterator rend() const;
     constexpr const_reverse_iterator crbegin() const;
     constexpr const_reverse_iterator crend() const;

private:
     const T* data_;
};

// =============================================================================
// Declare and define operators

#define DEFINE_UNARY_OP(OP, FUNC)                                              \
     template <typename T,                                                     \
               typename                                                        \
               = std::enable_if_t<details::is_valid_expr<T>::value, T>>        \
     constexpr auto operator OP(T&& rhs)                                       \
     {                                                                         \
          return FUNC(details::make_expr(rhs));                                \
     }
#define DEFINE_BINARY_OP(OP, FUNC)                                             \
     template <                                                                \
         typename T, typename U,                                               \
         typename = std::enable_if_t<details::is_valid_expr<T>::value, T>,     \
         typename = std::enable_if_t<details::is_valid_expr<U>::value, U>>     \
     constexpr auto operator OP(T&& lhs, U&& rhs)                              \
     {                                                                         \
          return FUNC(details::make_expr(lhs), details::make_expr(rhs));       \
     }

// Logical operators
DEFINE_UNARY_OP(~, logical_not)

DEFINE_BINARY_OP(&, logical_and)
DEFINE_BINARY_OP(|, logical_or)
DEFINE_BINARY_OP(^, logical_xor)

// Arithmetic operators
DEFINE_UNARY_OP(-, negate)

DEFINE_BINARY_OP(+, add)
DEFINE_BINARY_OP(-, sub)
DEFINE_BINARY_OP(*, mul)
DEFINE_BINARY_OP(/, div)

#undef DEFINE_UNARY_OP
#undef DEFINE_BINARY_OP
}  // namespace xsimd

#include "xsimd_fallback_batch_impl.hpp"

// =============================================================================
// Other XSIMD-related functions

namespace xsimd
{
#ifndef HAS_XSIMD
/*
 * Everything here is only used if XSIMD is not installed on the
 * system or if one does not want to include it.
 *
 * These functions/classes are typically only used to automatically
 * detect the optimal batch size for a given input.
 */
#     define DEFINE_SIMD_TRAITS_SPECIALISATION(atype, batch_size)              \
          template <>                                                          \
          struct simd_traits<atype>                                            \
          {                                                                    \
               static constexpr std::size_t size = batch_size;                 \
               using type = batch<atype, size>;                                \
          };                                                                   \
          template <>                                                          \
          struct const_simd_traits<atype>                                      \
          {                                                                    \
               static constexpr std::size_t size = batch_size;                 \
               using type = batch<const atype, size>;                          \
          }

//! Trait class to find optimal batch size given some arithmetic type
template <typename T>
struct simd_traits
{
     static constexpr std::size_t size = XSIMD_FALLBACK_DEFAULT_SIZE;
     using type = batch<T, size>;
};
template <typename T>
struct const_simd_traits
{
     static constexpr std::size_t size = simd_traits<T>::size;
     using type = batch<const T, size>;
};

DEFINE_SIMD_TRAITS_SPECIALISATION(int8_t, XSIMD_FALLBACK_BATCH_INT8_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(uint8_t, XSIMD_FALLBACK_BATCH_INT8_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(int16_t, XSIMD_FALLBACK_BATCH_INT16_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(uint16_t, XSIMD_FALLBACK_BATCH_INT16_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(int32_t, XSIMD_FALLBACK_BATCH_INT32_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(uint32_t, XSIMD_FALLBACK_BATCH_INT32_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(int64_t, XSIMD_FALLBACK_BATCH_INT64_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(uint64_t, XSIMD_FALLBACK_BATCH_INT64_SIZE);
DEFINE_SIMD_TRAITS_SPECIALISATION(float, XSIMD_FALLBACK_BATCH_FLOAT_SIZE);
#     ifdef XSIMD_BATCH_DOUBLE_SIZE
DEFINE_SIMD_TRAITS_SPECIALISATION(double, XSIMD_FALLBACK_BATCH_DOUBLE_SIZE);
#     endif  // XSIMD_BATCH_DOUBLE_SIZE

#     undef DEFINE_SIMD_TRAITS_SPECIALISATION

//! Convenience template alias
//! \ingroup xsimd_functions
template <typename T>
using simd_type = typename simd_traits<T>::type;

#ifndef DOXYGEN_DOC

//! \brief Load aligned data
//! \ingroup xsimd_functions
template <typename T>
auto load_aligned(T* t)
{
     return batch<T, simd_type<T>::size>(t);
}
//! \brief Load unaligned data
//! \ingroup xsimd_functions
template <typename T>
auto load_unaligned(T* t)
{
     return load_aligned(t);
}

#endif // DOXYGEN_DOC

//! Get the alignment offset of some memory
/*!
 *\ingroup xsimd_functions
 * Taken \e as-is from XSIMD
 */
template <typename T>
size_t get_alignment_offset(const T* p, size_t size, size_t block_size)
{
     if (block_size == 1) {
          return 0;
     }
     else if (size_t(p) & (sizeof(T) - 1)) {
          return size;
     }
     else {
          size_t block_mask = block_size - 1;
          return std::min<size_t>(
              (block_size - ((size_t(p) / sizeof(T)) & block_mask))
                  & block_mask,
              size);
     }
}

#else
template <typename T>
using const_simd_traits = simd_traits<T>;
#endif  // !HAS_XSIMD

/*!
 *\ingroup xsimd_functions
 * \brief Load unaligned data
 * Loads the memory array pointed to by \c src into a batch and returns it.
 * \c src is not required to be aligned.
 *
 * \param src the pointer to the memory array to load.
 * \param b the destination batch.
 *
 * \tparam T Type of memory to read from
 *
 * \note Uses SFINAE to remove this overload from resolution when using the
 *       real XSIMD
 */
template <typename T, typename batch_type,
          typename U = details::remove_cvref_t<batch_type>,
          typename = std::enable_if_t<
              details::is_batch<U>::value
                  && !std::is_convertible<U*, simd_base<U>*>::value,
              T>>
void load_unaligned(T* src, batch_type& b)
{
     load_aligned(src, b);
}
/*!
 *\ingroup xsimd_functions
 * \brief Load aligned data
 * Loads the memory array pointed to by \c src into a batch and returns it.
 * \c src is required to be aligned.
 *
 * \param src the pointer to the memory array to load.
 * \param b the destination batch.
 *
 * \tparam T Type of memory to read from
 *
 * \note Uses SFINAE to remove this overload from resolution when using the
 *       real XSIMD
 */
template <typename T, typename batch_type,
          typename U = details::remove_cvref_t<batch_type>,
          typename = std::enable_if_t<
              details::is_batch<U>::value
                  && !std::is_convertible<U*, simd_base<U>*>::value,
              T>>
void load_aligned(T* src, batch_type& b)
{
     b = batch<T, details::expr_size<U>::size>(src);
}

/*!
 * \brief Store unaligned data
 * Stores the expression \c expr into the memory array pointed to by \c dest.
 * \c dest is not required to be aligned.
 *
 * \param dest the pointer to the memory array.
 * \param expr the expression to store
 *
 * \tparam T Type of memory to read from
 *
 * \note Uses SFINAE to remove this overload from resolution when using the
 *       real XSIMD
 */
template <typename T, typename expr_type,
          typename U = details::remove_cvref_t<expr_type>,
          typename = std::enable_if_t<
              (details::is_batch<U>::value
               && !std::is_convertible<U*, simd_base<U>*>::value)
                  || std::is_convertible<U*, expr_base<U>*>::value,
              T>>
void store_unaligned(T* dest, expr_type&& expr)
{
     store_aligned(dest, std::forward<expr_type>(expr));
}
/*!
 *\ingroup xsimd_functions
 * \brief Store aligned data
 * Stores the expression \c expr into the memory array pointed to by \c dest.
 * \c dest is required to be aligned.
 *
 * \param dest the pointer to the memory array.
 * \param expr the expression to store
 *
 * \tparam T Type of memory to read from
 *
 * \note Uses SFINAE to remove this overload from resolution when using the
 *       real XSIMD
 */
template <typename T, typename expr_type,
          typename U = details::remove_cvref_t<expr_type>,
          typename = std::enable_if_t<
              (details::is_batch<U>::value
               && !std::is_convertible<U*, simd_base<U>*>::value)
                  || std::is_convertible<U*, expr_base<U>*>::value,
              T>>
void store_aligned(T* dest, expr_type&& expr)
{
     PRAGMA_LOOP_IVDEP
     for (std::size_t i(0); i < details::expr_size<U>::size; ++i) {
          dest[i] = details::make_expr(std::forward<expr_type>(expr))(
              eval_visitor(i));
     }
}

// =============================================================================

/*!
 * \brief In-place transform algorithm
 *
 * Same as xsimd::transform and std::transform, but the operations are
 * done in-place on the first argument
 *
 * \param first_1 Iterator to the beginning of first range
 * \param last_1 Iterator to the end of the first range
 * \param first_2 Iterator to the beginning of the second range
 * \param f Binary function or functor to apply to each element
 *
 * \note \c first_1 and \c last_1 must have read/write access
 *
 * \pre This function assumes that the alignment of both ranges is
 * identical and that they are properly aligned (ie. no alignment
 * offset to the beginning of the data)
 */
template <typename I1, typename I2, typename I3, typename UF>
void self_transform(I1 first_1, I2 last_1, I3 first_2, UF&& f)
{
     using value_type = typename std::decay<decltype(*first_1)>::type;
     using traits = simd_traits<value_type>;
     using batch_type = typename traits::type;

     auto size = static_cast<std::size_t>(std::distance(first_1, last_1));
     constexpr auto simd_size = traits::size;

     auto align_begin_1
         = xsimd::get_alignment_offset(&(*first_1), size, simd_size);
     auto align_begin_2
         = xsimd::get_alignment_offset(&(*first_2), size, simd_size);
     auto align_end
         = align_begin_1 + ((size - align_begin_1) & ~(simd_size - 1));

     assert(align_begin_1 == align_begin_2);
     assert(align_begin_1 == 0);

     batch_type batch_1, batch_2;
     PRAGMA_LOOP_IVDEP
     for (std::size_t i = align_begin_1; i < align_end; i += simd_size) {
          xsimd::load_aligned(&first_1[i], batch_1);
          xsimd::load_aligned(&first_2[i], batch_2);
          xsimd::store_aligned(&first_1[i], f(batch_1, batch_2));
     }

     PRAGMA_LOOP_IVDEP
     for (std::size_t i = align_end; i < size; ++i) {
          first_1[i] = f(first_1[i], first_2[i]);
     }
}
}  // namespace xsimd
