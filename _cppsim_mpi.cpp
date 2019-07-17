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

#include <pybind11/complex.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include <complex>
#include <iostream>
#include <vector>
#if defined(_OPENMP)
#     include <omp.h>
#endif
#include <boost/container/vector.hpp>

#include "simulator-mpi/SimulatorMPI.hpp"

namespace pybind11
{
namespace detail
{
     template <typename Type, typename Alloc>
     struct type_caster<boost::container::vector<Type, Alloc>>
         : list_caster<boost::container::vector<Type, Alloc>, Type>
     {};
}  // namespace detail
}  // namespace pybind11

namespace py = pybind11;

using c_type = std::complex<double>;
using ArrayType = std::vector<c_type, aligned_allocator<c_type, 64>>;
using MatrixType = std::vector<ArrayType>;
using QuRegs = std::vector<std::vector<unsigned>>;

template <class QR>
void emulate_math_wrapper(SimulatorMPI& sim, py::function const& pyfunc,
                          QR const& qr, Fusion::IndexVector const& ctrls)
{
     auto f = [&](std::vector<int>& x) {
          pybind11::gil_scoped_acquire acquire;
          x = std::move(pyfunc(x).cast<std::vector<int>>());
     };
     pybind11::gil_scoped_release release;
     sim.emulate_math(f, qr, ctrls);
}

PYBIND11_MODULE(_cppsim_mpi, m)
{
     py::class_<SimulatorMPI>(m, "SimulatorMPI")
         .def(py::init<uint64_t, int, int>())
         .def("get_qubits_ids", &SimulatorMPI::GetQubitsPermutation)
         .def("get_local_qubits_ids", &SimulatorMPI::GetLocalQubitsPermutation)
         .def("get_global_qubits_ids",
              &SimulatorMPI::GetGlobalQubitsPermutation)
         .def("set_qubits_perm", &SimulatorMPI::SetQubitsPermutation)
         .def("swap_qubits", &SimulatorMPI::SwapQubitsWrapper)
         .def("allocate_qureg", &SimulatorMPI::AllocateQureg)
         .def("allocate_qubit", &SimulatorMPI::AllocateQubit)
         .def("deallocate_qubit", &SimulatorMPI::DeallocateQubit)
         .def("measure_qubits", &SimulatorMPI::MeasureQubits)
         .def("apply_controlled_gate", &SimulatorMPI::ApplyGate)
         .def("emulate_math", &emulate_math_wrapper<QuRegs>)
         .def("get_amplitude", &SimulatorMPI::GetAmplitude)
         .def("get_probability", &SimulatorMPI::GetProbability)
         .def("run", &SimulatorMPI::Run)
         .def("entropy", &SimulatorMPI::Entropy)
         .def("cheat_local", &SimulatorMPI::cheat_local)
         .def("collapse_wavefunction", &SimulatorMPI::collapseWaveFunction);
}
