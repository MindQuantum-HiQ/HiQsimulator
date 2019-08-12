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

//! Default constructor
template <typename T, std::size_t N>
xsimd::batch<T, N>::batch() : array_data_{}
{}

//! Constructor from an array of values
/*!
 * Initializes a batch to the values pointed by \c data.
 */
template <typename T, std::size_t N>
xsimd::batch<T, N>::batch(T data[N])
    : array_data_{details::array_from_pointer<T, N>(data)}
{}

//! Constructor from an array of values
/*!
 * Initializes a batch to the values pointed by \c data.
 */
template <typename T, std::size_t N>
xsimd::batch<T, N>::batch(const std::array<T, N>& data) : array_data_{data}
{}

//! Constructor from scalar values
/*!
 * Initializes all the values of the batch to \c val.
 */
template <typename T, std::size_t N>
xsimd::batch<T, N>::batch(T val)
    : array_data_(details::array_from_scalar<T, N>(val))
{}

// -----------------------------------------------------------------------------

//! Access element (const)
/*!
 * \param idx Index of an element in the batch
 * \return The element at the specified position
 */
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::operator[](std::size_t idx) const
{
     return array_data_[idx];
}

//! Access element
/*!
 * \param idx Index of an element in the batch
 * \return A reference to the element at the specified position
 */
template <typename T, std::size_t N>
auto& xsimd::batch<T, N>::operator[](std::size_t idx)
{
     return array_data_[idx];
}

// =============================================================================
// Assignment operators

//! Assignment operator
/*!
 * Assignment operator for batches of constant values.
 *
 * \param rhs Batch of constant values
 * \return A reference to \c this.
 */
template <typename T, std::size_t N>
auto& xsimd::batch<T, N>::operator=(const batch<const T, N>& rhs)
{
     PRAGMA_LOOP_IVDEP
     for (std::size_t i(0); i < size; ++i) {
          array_data_[i] = rhs[i];
     }
     return *this;
}

//! Move assignment operator
/*!
 * Move assignment operator for batches of constant values.
 *
 * \param rhs R-value of batch of constant values
 * \return A reference to \c this.
 */
template <typename T, std::size_t N>
auto& xsimd::batch<T, N>::operator=(batch<const T, N>&& rhs)
{
     PRAGMA_LOOP_IVDEP
     for (std::size_t i(0); i < size; ++i) {
          array_data_[i] = rhs[i];
     }
     return *this;
}

//! Assignment operator for arbitrary expressions
/*!
 * \param rhs RHS expression
 * \return A reference to \c this.
 */
template <typename T, std::size_t N>
template <typename U, typename>
auto& xsimd::batch<T, N>::operator=(U&& rhs)
{
     static_assert(size == details::expr_size<U>::size,
                   "Expression size must match batch size");
     /*
      * This can lead to near-optimal code if the compiler is
      * allowed to vectorize and unroll the loop automatically
      */
     PRAGMA_LOOP_IVDEP
     for (std::size_t i(0); i < size; ++i) {
          array_data_[i]
              = details::make_expr(std::forward<U>(rhs))(eval_visitor(i));
     }
     return *this;
}

// =============================================================================
// Inplace operators

#define DEFINE_INPLACE_OP(OP)                                                  \
     template <typename T, std::size_t N>                                      \
     template <typename U, typename>                                           \
     auto& xsimd::batch<T, N>::operator OP(U&& rhs)                            \
     {                                                                         \
          static_assert(std::is_arithmetic<details::remove_cvref_t<U>>::value  \
                            || size == details::expr_size<U>::size,            \
                        "Expression size must match batch size");              \
                                                                               \
          PRAGMA_LOOP_IVDEP                                                    \
          for (std::size_t i(0); i < size; ++i) {                              \
               array_data_[i] OP details::make_expr(std::forward<U>(rhs))(     \
                   eval_visitor(i));                                           \
          }                                                                    \
          return *this;                                                        \
     }

DEFINE_INPLACE_OP(+=)
DEFINE_INPLACE_OP(-=)
DEFINE_INPLACE_OP(*=)
DEFINE_INPLACE_OP(/=)

DEFINE_INPLACE_OP(&=)
DEFINE_INPLACE_OP(|=)
DEFINE_INPLACE_OP(^=)
#undef DEFINE_INPLACE_OP

//! Pre-increment operator.
/*!
 * \return A reference to \c this.
 */
template <typename T, std::size_t N>
auto& xsimd::batch<T, N>::operator++()
{
     PRAGMA_LOOP_IVDEP
     for (auto& b: *this) {
          ++b;
     }
     return *this;
}
//! Post-increment operator.
/*!
 * \return A reference to \c this.
 */
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::operator++(int)
{
     auto tmp(*this);
     ++(*this);
     return tmp;
}
//! Pre-decrement operator.
/*!
 * \return A reference to \c this.
 */
template <typename T, std::size_t N>
auto& xsimd::batch<T, N>::operator--()
{
     PRAGMA_LOOP_IVDEP
     for (auto& b: *this) {
          --b;
     }
     return *this;
}
//! Post-decrement operator.
/*!
 * \return A reference to \c this.
 */
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::operator--(int)
{
     auto tmp(*this);
     --(*this);
     return tmp;
}

// =============================================================================
// Function for iterator-like access

//! Returns iterator to beginning
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::begin() -> iterator
{
     return array_data_.begin();
}
//! Returns iterator to end
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::end() -> iterator
{
     return array_data_.end();
}
//! Returns const_iterator to beginning
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::begin() const -> const_iterator
{
     return cbegin();
}
//! Returns const_iterator to end
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::end() const -> const_iterator
{
     return cend();
}
//! Returns const_iterator to beginning
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::cbegin() const -> const_iterator
{
     return array_data_.cbegin();
}
//! Returns const_iterator to end
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::cend() const -> const_iterator
{
     return array_data_.cend();
}

// ------------------------------------------------------------------------------

//! Return reverse_iterator to reverse beginning
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::rbegin() -> reverse_iterator
{
     return reverse_iterator(end());
}
//! Return reverse_iterator to reverse end
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::rend() -> reverse_iterator
{
     return reverse_iterator(begin());
}
//! Return const_reverse_iterator to reverse beginning
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::rbegin() const -> const_reverse_iterator
{
     return crbegin();
}
//! Return const_reverse_iterator to reverse end
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::rend() const -> const_reverse_iterator
{
     return crend();
}
//! Return const_reverse_iterator to reverse beginning
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::crbegin() const -> const_reverse_iterator
{
     return const_reverse_iterator(cend());
}
//! Return const_reverse_iterator to reverse end
template <typename T, std::size_t N>
auto xsimd::batch<T, N>::crend() const -> const_reverse_iterator
{
     return const_reverse_iterator(cbegin());
}

// =============================================================================
// =============================================================================
// Constructor for const batches

template <typename T, std::size_t N>
constexpr xsimd::batch<const T, N>::batch() : data_(nullptr)
{}

template <typename T, std::size_t N>
constexpr xsimd::batch<const T, N>::batch(const T data[N]) : data_(data)
{}

template <typename T, std::size_t N>
constexpr xsimd::batch<const T, N>::batch(const std::array<T, N>& data)
    : data_(&data[0])
{}

template <typename T, std::size_t N>
constexpr xsimd::batch<const T, N>::batch(const batch& rhs) : data_(rhs.data_)
{}

template <typename T, std::size_t N>
constexpr xsimd::batch<const T, N>::batch(batch&& rhs)
    : data_(std::move(rhs.data_))
{}

// -----------------------------------------------------------------------------

template <typename T, std::size_t N>
constexpr auto& xsimd::batch<const T, N>::operator=(const batch& rhs)
{
     data_ = rhs.data_;
     return *this;
}

template <typename T, std::size_t N>
constexpr auto& xsimd::batch<const T, N>::operator=(batch&& rhs)
{
     data_ = std::move(rhs.data_);
     return *this;
}

// -----------------------------------------------------------------------------

//! Access element (const)
/*!
 * \param idx Index of an element in the batch
 * \return The element at the specified position
 */
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::operator[](std::size_t idx) const
{
     return data_[idx];
}

// =============================================================================
// Function for iterator-like access

//! Returns const_iterator to beginning
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::begin() const -> const_iterator
{
     return cbegin();
}
//! Returns const_iterator to end
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::end() const -> const_iterator
{
     return cend();
}
//! Returns const_iterator to beginning
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::cbegin() const -> const_iterator
{
     return data_;
}
//! Returns const_iterator to end
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::cend() const -> const_iterator
{
     return data_ + size;
}

// -----------------------------------------------------------------------------

//! Return const_reverse_iterator to reverse beginning
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::rbegin() const
    -> const_reverse_iterator
{
     return crbegin();
}
//! Return const_reverse_iterator to reverse end
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::rend() const -> const_reverse_iterator
{
     return crend();
}
//! Return const_reverse_iterator to reverse beginning
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::crbegin() const
    -> const_reverse_iterator
{
     return const_reverse_iterator(cend());
}
//! Return const_reverse_iterator to reverse end
template <typename T, std::size_t N>
constexpr auto xsimd::batch<const T, N>::crend() const -> const_reverse_iterator
{
     return const_reverse_iterator(cbegin());
}
