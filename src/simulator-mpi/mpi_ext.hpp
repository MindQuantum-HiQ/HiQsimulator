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

#ifndef MPI_EXT_HPP
#define MPI_EXT_HPP

#include <boost/mpi.hpp>
#include <complex>

using dcomplex = std::complex<double>;
using dcplus = std::plus<dcomplex>;

namespace boost
{
namespace mpi
{
     template <>
     inline MPI_Datatype get_mpi_datatype<dcomplex>(const dcomplex&)
     {
          return MPI_DOUBLE_COMPLEX;
     }
     template <>
     struct is_mpi_complex_datatype<dcomplex> : boost::mpl::true_
     {};
}  // namespace mpi
}  // namespace boost

// optional; works as expected for assertion (4)
namespace boost
{
namespace mpi
{
     template <>
     struct is_commutative<dcplus, dcomplex> : mpl::true_
     {};
}  // namespace mpi
}  // namespace boost

#endif  // MPI_EXT_HPP
