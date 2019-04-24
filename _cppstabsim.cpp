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

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include <pybind11/pytypes.h>

#include "stabilizer-simulator/StabilizerSimulator.hpp"

PYBIND11_MODULE(_cppstabsim, m) {
    pybind11::class_<StabilizerSimulator>(m, "StabilizerSimulator")
        .def(pybind11::init<>())
        .def("sync", &StabilizerSimulator::sync)
        .def("allocate_qureg", &StabilizerSimulator::AllocateQureg)
        .def("h", &StabilizerSimulator::h)
        .def("s", &StabilizerSimulator::s)
        .def("cnot", &StabilizerSimulator::cnot)
        .def("measure", &StabilizerSimulator::measure)
        ;
}
