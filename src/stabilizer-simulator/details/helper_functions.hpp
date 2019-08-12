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

#include <array>
#include <functional>

namespace details
{
//! Trait class to identify array types
template <typename T>
struct array_traits;

//! Specialisation to identify std::array
template <typename T, std::size_t N>
struct array_traits<std::array<T, N>>
{
     using type = T;
     static constexpr auto size = N;
};
//! Specialisation to identify C-style arrays
template <typename T, std::size_t N>
struct array_traits<T[N]>
{
     using type = T;
     static constexpr auto size = N;
};

//! Apply a function to each element of an array while accumulating the result
/*!
 * \c func_t must be a functor class which implements the ()-operator:
 * \code
 T func_t::operator()( const T& lhs, const T& rhs ) const;
 * \endcode
 *
 * \param array Array of value to apply the function to
 * \param val Value used to accumulate the result
 *
 * \tparam func_t Class implementing a function via the ()-operator
 * \tparam array_t Array class
 */
template <typename func_t, typename array_t>
constexpr void apply_array_func(const array_t& array,
                                typename array_traits<array_t>::type& val)
{
     for (const auto& el: array) {
          val = func_t()(val, el);
     }
}

//! Apply a function to each element of an array while accumulating the result
/*!
 * \c func_t must be a functor class which implements the ()-operator:
 * \code
 T func_t::operator()( const T& lhs, const T& rhs ) const;
 * \endcode
 *
 * \param array Array of value to apply the function to
 *
 * \tparam func_t Class implementing a function via the ()-operator
 * \tparam array_t Array class
 */
template <typename func_t, typename array_t>
constexpr void apply_array_func(const array_t& array)
{
     using value_type = typename array_traits<array_t>::type;
     auto val = value_type();
     apply_array_func(array, val);
     return val;
}
}  // namespace details
