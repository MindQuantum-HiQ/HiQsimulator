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

// Force the use of our XSIMD replacement library
#ifdef HAS_XSIMD
#     undef HAS_XSIMD
#endif  // HAS_XSIMD
#include "xsimd_fallback/xsimd_fallback.hpp"

#define BOOST_TEST_MODULE xsimd_fallback_test
#define BOOST_TEST_DYN_LINK
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <type_traits>
#include <utility>

static_assert(
    !std::is_convertible<xsimd::batch<uint32_t, 8>*,
                         xsimd::simd_base<xsimd::batch<uint32_t, 8>>*>::value,
    "Testing original XSIMD: something's wrong");

BOOST_AUTO_TEST_CASE(batch_default_init)
{
     uint32_t zeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};
     xsimd::batch<uint32_t, 8> batch;
     BOOST_TEST(batch == zeros, boost::test_tools::per_element());

     double zerosd[8] = {0., 0., 0., 0., 0., 0., 0., 0.};
     xsimd::batch<double, 8> batchd;
     BOOST_TEST(batchd == zeros, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(batch_scalar_init)
{
     uint32_t ones[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
     xsimd::batch<uint32_t, 10> batch{1};
     BOOST_TEST(batch == ones, boost::test_tools::per_element());

     double twos[10] = {2., 2., 2., 2., 2., 2., 2., 2., 2., 2.};
     xsimd::batch<double, 10> batchd{2.};
     BOOST_TEST(batchd == twos, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(batch_array_init)
{
     uint32_t data[5] = {1, 2, 3, 4, 5};
     xsimd::batch<uint32_t, 5> batch{data};
     BOOST_TEST(batch == data, boost::test_tools::per_element());

     std::array<uint32_t, 5> array = {1, 2, 3, 4, 5};
     xsimd::batch<uint32_t, 5> batch2{array};
     BOOST_TEST(batch2 == array, boost::test_tools::per_element());

     double datad[5] = {1., 2., 3., -4., 5.};
     xsimd::batch<double, 5> batchd{datad};
     BOOST_TEST(batchd == datad, boost::test_tools::per_element());

     std::array<double, 5> arrayd = {1., 2., 3., -4., 5.};
     xsimd::batch<double, 5> batchd2{arrayd};
     BOOST_TEST(batchd2 == arrayd, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(batch_index_operator)
{
     std::array<uint32_t, 5> data = {1, 2, 3, 4, 5};
     xsimd::batch<uint32_t, 5> batch{data};
     const xsimd::batch<uint32_t, 5>& cbatch{batch};

     for (auto i(0UL); i < data.size(); ++i) {
          BOOST_TEST(batch[i] == data[i]);
          BOOST_TEST(cbatch[i] == data[i]);
     }
}

BOOST_AUTO_TEST_CASE(batch_assignment_operator)
{
     std::array<uint32_t, 5> ref1 = {1, 2, 3, 4, 5};
     std::array<uint32_t, 5> ref2 = {10, 20, 30, 40, 50};

     xsimd::batch<uint32_t, 5> batch1;
     xsimd::batch<const uint32_t, 5> batch2{ref1};

     BOOST_TEST(batch1 != batch2, boost::test_tools::per_element());
     batch1 = batch2;
     BOOST_TEST(batch1 == batch2, boost::test_tools::per_element());
     batch1 = xsimd::batch<const uint32_t, 5>(ref2);
     BOOST_TEST(batch1 == ref2, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(batch_expression_assignment_operator)
{
     std::array<uint32_t, 5> ref1 = {1, 2, 3, 4, 5};
     std::array<uint32_t, 5> ref2 = {9, 18, 27, 36, 45};
     std::array<uint32_t, 5> ref3 = {10, 20, 30, 40, 50};

     xsimd::batch<uint32_t, 5> batch1{ref1};
     xsimd::batch<uint32_t, 5> batch2{ref2};

     xsimd::batch<uint32_t, 5> batch{ref1};
     batch = batch1 + batch2;
     BOOST_TEST(batch == ref3, boost::test_tools::per_element());
     batch = batch1 * 10;
     BOOST_TEST(batch == ref3, boost::test_tools::per_element());
     batch = batch1 + 2 * batch2 - batch2;
     BOOST_TEST(batch == ref3, boost::test_tools::per_element());
     auto expr = batch1 + 2 * batch2 - batch2;
     batch = expr;
     BOOST_TEST(batch == ref3, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(batch_iterator_access)
{
     std::array<uint32_t, 5> data = {1, 2, 3, 4, 5};
     xsimd::batch<uint32_t, 5> batch{data};
     const xsimd::batch<uint32_t, 5>& cbatch{batch};

     {
          auto it = data.begin();
          for (auto b_it = batch.begin(); b_it != batch.end(); ++b_it, ++it) {
               BOOST_TEST(*b_it == *it);
          }
     }
     {
          auto it = data.cbegin();
          for (auto b_it = batch.cbegin(); b_it != batch.cend(); ++b_it, ++it) {
               BOOST_TEST(*b_it == *it);
          }
     }
     {
          auto it = data.cbegin();
          for (auto b_it = cbatch.begin(); b_it != cbatch.end(); ++b_it, ++it) {
               BOOST_TEST(*b_it == *it);
          }
     }

     {
          auto it = data.rbegin();
          for (auto b_it = batch.rbegin(); b_it != batch.rend(); ++b_it, ++it) {
               BOOST_TEST(*b_it == *it);
          }
     }
     {
          auto it = data.crbegin();
          for (auto b_it = batch.crbegin(); b_it != batch.crend();
               ++b_it, ++it) {
               BOOST_TEST(*b_it == *it);
          }
     }
     {
          auto it = data.crbegin();
          for (auto b_it = cbatch.rbegin(); b_it != cbatch.rend();
               ++b_it, ++it) {
               BOOST_TEST(*b_it == *it);
          }
     }
}

BOOST_AUTO_TEST_CASE(batch_inplace_operators)
{
     std::array<uint32_t, 5> zeros = {0, 0, 0, 0, 0};
     std::array<uint32_t, 5> ones = {1, 1, 1, 1, 1};
     std::array<uint32_t, 5> twos = {2, 2, 2, 2, 2};
     std::array<uint32_t, 5> fours = {4, 4, 4, 4, 4};

     xsimd::batch<uint32_t, 5> batch;
     xsimd::batch<uint32_t, 5> bfours{fours};
     xsimd::batch<uint32_t, 5> btwos{twos};
     batch += 4;
     BOOST_TEST(batch == fours, boost::test_tools::per_element());
     batch -= 2;
     BOOST_TEST(batch == twos, boost::test_tools::per_element());
     batch /= 2;
     BOOST_TEST(batch == ones, boost::test_tools::per_element());
     batch *= 4;
     BOOST_TEST(batch == fours, boost::test_tools::per_element());

     batch &= 2;
     BOOST_TEST(batch == zeros, boost::test_tools::per_element());
     batch |= 4;
     BOOST_TEST(batch == fours, boost::test_tools::per_element());
     batch ^= 4;
     BOOST_TEST(batch == zeros, boost::test_tools::per_element());

     batch += bfours;
     BOOST_TEST(batch == fours, boost::test_tools::per_element());
     batch -= btwos;
     BOOST_TEST(batch == twos, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(batch_increment_decrement)
{
     std::array<uint32_t, 5> zeros = {0, 0, 0, 0, 0};
     std::array<uint32_t, 5> ones = {1, 1, 1, 1, 1};
     std::array<uint32_t, 5> twos = {2, 2, 2, 2, 2};

     xsimd::batch<uint32_t, 5> batch;
     ++batch;
     BOOST_TEST(batch == ones, boost::test_tools::per_element());
     auto tmp = batch++;
     BOOST_TEST(tmp == ones, boost::test_tools::per_element());
     BOOST_TEST(batch == twos, boost::test_tools::per_element());

     --batch;
     BOOST_TEST(batch == ones, boost::test_tools::per_element());
     auto tmp2 = batch--;
     BOOST_TEST(tmp2 == ones, boost::test_tools::per_element());
     BOOST_TEST(batch == zeros, boost::test_tools::per_element());
}

// =============================================================================

BOOST_AUTO_TEST_CASE(const_batch_default_init)
{
     constexpr xsimd::batch<const uint32_t, 8> batch;
     BOOST_TEST(batch.begin() == nullptr);

     constexpr xsimd::batch<const double, 8> batchd;
     BOOST_TEST(batchd.begin() == nullptr);
}

BOOST_AUTO_TEST_CASE(const_batch_array_init)
{
     static constexpr uint32_t data[5] = {1, 2, 3, 4, 5};
     constexpr xsimd::batch<const uint32_t, 5> batch{data};
     BOOST_TEST(batch == data, boost::test_tools::per_element());

     static constexpr std::array<uint32_t, 5> array = {1, 2, 3, 4, 5};
     constexpr xsimd::batch<const uint32_t, 5> batch2{array};
     BOOST_TEST(batch2 == array, boost::test_tools::per_element());

     static constexpr double datad[5] = {1., 2., 3., -4., 5.};
     constexpr xsimd::batch<const double, 5> batchd{datad};
     BOOST_TEST(batchd == datad, boost::test_tools::per_element());

     static constexpr std::array<double, 5> arrayd = {1., 2., 3., -4., 5.};
     constexpr xsimd::batch<const double, 5> batchd2{arrayd};
     BOOST_TEST(batchd2 == arrayd, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(const_batch_copy_move_init)
{
     static constexpr uint32_t data[5] = {1, 2, 3, 4, 5};
     constexpr xsimd::batch<const uint32_t, 5> batch{data};

     constexpr auto batch2 = batch;
     BOOST_TEST(batch == batch2, boost::test_tools::per_element());
     BOOST_TEST(batch2 == data, boost::test_tools::per_element());

     constexpr auto batch3 = xsimd::batch<const uint32_t, 5>{data};
     BOOST_TEST(batch == batch3, boost::test_tools::per_element());
     BOOST_TEST(batch3 == data, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(const_batch_index_operator)
{
     static constexpr uint32_t data[5] = {1, 2, 3, 4, 5};
     constexpr xsimd::batch<const uint32_t, 5> batch{data};
     BOOST_TEST(batch[0] == data[0]);
     BOOST_TEST(batch[1] == data[1]);
     BOOST_TEST(batch[2] == data[2]);
     BOOST_TEST(batch[3] == data[3]);
     BOOST_TEST(batch[4] == data[4]);
}

BOOST_AUTO_TEST_CASE(const_batch_iterator_access)
{
     static constexpr std::array<uint32_t, 5> data = {1, 2, 3, 4, 5};
     constexpr xsimd::batch<const uint32_t, 5> batch{data};
     BOOST_TEST(&*batch.begin() == &*data.begin());
     BOOST_TEST(&*batch.cbegin() == &*data.cbegin());
     BOOST_TEST(&*batch.end() == &*data.end());
     BOOST_TEST(&*batch.cend() == &*data.cend());

     BOOST_TEST(&*batch.rbegin() == &*data.rbegin());
     BOOST_TEST(&*batch.crbegin() == &*data.crbegin());
     BOOST_TEST(&*batch.rend() == &*data.rend());
     BOOST_TEST(&*batch.crend() == &*data.crend());
}

// =============================================================================

BOOST_AUTO_TEST_CASE(xsimd_load_save)
{
     std::array<uint32_t, 5> zeros = {0, 0, 0, 0, 0};
     std::array<uint32_t, 5> ones = {1, 1, 1, 1, 1};
     std::array<uint32_t, 5> twos = {2, 2, 2, 2, 2};

     std::array<uint32_t, 5> res1;
     std::array<uint32_t, 5> res2;

     xsimd::batch<uint32_t, 5> batch;
     xsimd::load_aligned(ones.data(), batch);
     BOOST_TEST(batch == ones, boost::test_tools::per_element());

     xsimd::load_unaligned(twos.data(), batch);
     BOOST_TEST(batch == twos, boost::test_tools::per_element());

     batch = batch - 1;
     xsimd::store_aligned(res1.data(), batch);
     BOOST_TEST(res1 == ones, boost::test_tools::per_element());

     xsimd::store_unaligned(res2.data(), batch);
     BOOST_TEST(res2 == ones, boost::test_tools::per_element());
}

// =============================================================================

BOOST_AUTO_TEST_CASE(xsimd_eval_visitor)
{
     using xsimd::eval_visitor;
     namespace tags = xsimd::tags;

     std::array<uint32_t, 5> data = {1, 2, 3, 4, 5};
     xsimd::batch<uint32_t, 5> batch{data};

     for (auto i(0UL); i < data.size(); ++i) {
          const auto visitor = eval_visitor(i);
          BOOST_TEST(eval_visitor(i)(tags::cst_tag{}, i) == i);
          BOOST_TEST(eval_visitor(i)(tags::batch_tag{}, batch) == batch[i]);

          BOOST_TEST(eval_visitor(i)(tags::negate_tag{}, i) == -i);
          BOOST_TEST(eval_visitor(i)(tags::add_tag{}, i, 10) == i + 10);
          BOOST_TEST(eval_visitor(i)(tags::sub_tag{}, 100, i) == 100 - i);
          BOOST_TEST(eval_visitor(i)(tags::mul_tag{}, i, 7) == i * 7);
          BOOST_TEST(eval_visitor(i)(tags::div_tag{}, 100, i + 1)
                     == 100 / (i + 1));

          BOOST_TEST(eval_visitor(i)(tags::logical_not_tag{}, i) == ~i);
          BOOST_TEST(eval_visitor(i)(tags::logical_and_tag{}, i, 10)
                     == (i & 10));
          BOOST_TEST(eval_visitor(i)(tags::logical_or_tag{}, i, 10)
                     == (i | 10));
          BOOST_TEST(eval_visitor(i)(tags::logical_xor_tag{}, i, 10)
                     == (i ^ 10));
     }
}

// ==============================================================================

BOOST_AUTO_TEST_CASE(xsimd_expression_simple)
{
     std::array<uint32_t, 5> data = {1, 2, 3, 4, 5};
     xsimd::batch<const uint32_t, 5> batch{data.data()};

     for (auto i(0UL); i < data.size(); ++i) {
          BOOST_TEST(xsimd::details::make_expr(10)(xsimd::eval_visitor(i))
                     == 10);
          BOOST_TEST(xsimd::details::make_expr(batch)(xsimd::eval_visitor(i))
                     == data[i]);
     }
}

BOOST_AUTO_TEST_CASE(xsimd_expression_complex)
{
     std::array<uint32_t, 5> data = {1, 2, 3, 4, 5};
     std::array<uint32_t, 5> data2 = {1000, 2000, 3000, 4000, 5000};
     std::array<uint32_t, 5> ref;
     xsimd::batch<const uint32_t, 5> batch{data.data()};
     xsimd::batch<const uint32_t, 5> batch2{data2.data()};

     auto foo = [](auto a, auto b) { return (a + b) ^ (a - b); };

     auto expr = foo(batch, batch2);
     for (auto i(0UL); i < data.size(); ++i) {
          BOOST_TEST(expr(xsimd::eval_visitor(i)) == foo(data[i], data2[i]));
     }

     // --------------------------------

     auto goo = [](auto a, auto b) { return (a * b) | (b / a); };

     auto expr2 = goo(batch, batch2);
     for (auto i(0UL); i < data.size(); ++i) {
          BOOST_TEST(expr2(xsimd::eval_visitor(i)) == goo(data[i], data2[i]));
     }

     // --------------------------------

     auto hoo = [](auto a, auto b) { return (~a + (-b)) | (b ^ a); };

     auto expr3 = hoo(batch, batch2);
     for (auto i(0UL); i < data.size(); ++i) {
          BOOST_TEST(expr3(xsimd::eval_visitor(i)) == hoo(data[i], data2[i]));
     }
}
