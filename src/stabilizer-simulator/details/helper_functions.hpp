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
template <typename T>
struct array_traits;

template <typename T, std::size_t N>
struct array_traits<std::array<T, N>>
{
     using type = T;
     static constexpr auto size = N;
};
template <typename T, std::size_t N>
struct array_traits<T[N]>
{
     using type = T;
     static constexpr auto size = N;
};

template <typename func_t, typename array_t>
constexpr auto apply_array_func(const array_t& array,
                                typename array_traits<array_t>::type& val)
{
     for (const auto& el: array) {
          val = func_t()(val, el);
     }
}
template <typename func_t, typename array_t>
constexpr auto apply_array_func(const array_t& array)
{
     using value_type = typename array_traits<array_t>::type;
     auto val = value_type();
     apply_array_func(array, val);
     return val;
}
}  // namespace details
