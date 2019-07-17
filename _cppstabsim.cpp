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
#include "stabilizer-simulator/StabilizerSimulator.hpp"

namespace py = pybind11;

using c_type = std::complex<double>;
using ArrayType = std::vector<c_type, xsimd::aligned_allocator<c_type, 64>>;
using MatrixType = std::vector<ArrayType>;
using QuRegs = std::vector<std::vector<unsigned>>;

PYBIND11_MODULE(_cppstabsim, m)
{
     py::class_<StabilizerSimulator>(m, "Simulator")
         .def(py::init<unsigned, unsigned>())
         .def("X", &StabilizerSimulator::X)
         .def("S", &StabilizerSimulator::S)
         .def("H", &StabilizerSimulator::H)
         .def("CNOT", &StabilizerSimulator::CNOT)
         .def("allocate_qubit", &StabilizerSimulator::allocate_qubit)
         .def("deallocate_qubit", &StabilizerSimulator::deallocate_qubit)
         .def("get_classical_value", &StabilizerSimulator::get_classical_value)
         .def("is_classical", &StabilizerSimulator::is_classical)
         .def("measure_qubits", &StabilizerSimulator::measure_qubits_return)
         .def("get_probability", &StabilizerSimulator::get_probability)
         .def("collapse_wavefunction",
              &StabilizerSimulator::collapse_wavefunction)
         .def("set_qubits", &StabilizerSimulator::set_qubits)
         .def("run", &StabilizerSimulator::sync);
}
