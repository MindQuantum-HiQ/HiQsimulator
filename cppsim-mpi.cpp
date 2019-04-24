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

#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <cstdint>
#include <chrono>
#include <complex>

#include "simulator-mpi/SimulatorMPI.hpp"
#include "simulator-mpi/funcs.hpp"
#include <boost/format.hpp>
#include <boost/container/vector.hpp>
#include <omp.h>

#include <glog/logging.h>

using std::cout;
using std::cerr;
using std::endl;
using namespace boost;

namespace bc = boost::container;
namespace po = boost::program_options;


const double sqrt2 = 1.414213562373095;
Fusion::Matrix
H = { {1.0/sqrt2,  1.0/sqrt2},
      {1.0/sqrt2, -1.0/sqrt2} };


Fusion::Matrix
X = { {0.0, 1.0},
      {1.0, 0.0} };

Fusion::Matrix
I = { {1.0, 0.0},
      {0.0, 1.0} };

Fusion::Matrix
I2 = { {1.0, 0.0, 0.0, 0.0},
       {0.0, 1.0, 0.0, 0.0},
       {0.0, 0.0, 1.0, 0.0},
       {0.0, 0.0, 0.0, 1.0} };

Fusion::Matrix
RZpi = { {{1.0/sqrt2, 1.0/sqrt2},  0.0},
         {0.0, {-1.0/sqrt2, -1.0/sqrt2}} };

Fusion::Matrix
RZpi2 = { {1.0,  0.0},
          {0.0, {0.0, -1.0}} };

void printMeasuredQureg(std::vector<int> const& v) {
    std::string s;
    s.resize(v.size());
    std::transform(v.begin(), v.end(), s.rbegin(), [](int a) -> char { return a == 0 ? '0' : a == 1 ? '1' : 'x'; } );
    cout << "Measured qureg: " << s << endl;
}

SimulatorMPI::StateVector allocateData1(int rank, uint64_t M) {
    SimulatorMPI::StateVector res(1ul << M);
    for(size_t i = 0; i < res.size(); ++i)
        res[i] = std::complex<double>(rank, i);

    return res;
}

bool checkAmplitudePlacement(uint64_t M, uint64_t rank, uint64_t index, const std::complex<double>& a, const std::vector<int64_t>& iperm) {

    uint64_t L = iperm.size();

    uint64_t naturalIndex = 0;
    for(size_t bit = 0; bit < M; ++bit) {
        bool bv = index & (1ul << bit);
        naturalIndex |= bv*(1ul << iperm[bit]);
    }

    uint64_t Nglobal = L - M;
    for(size_t rbit = 0; rbit < Nglobal; ++rbit) {
        bool bv = rank & (1ul << rbit);
        naturalIndex |= bv*(1ul << iperm[rbit+M]);
    }

    uint64_t aIndex = uint64_t(a.real())*(1ul << M) + uint64_t(a.imag());

    if(naturalIndex != aIndex) {
        LOG(ERROR) << format("checkAmplitudePlacement(): amp: %f, index: %s, naturalIndex != aIndex: %s != %s")
        % a % bitstring(index, L) % bitstring(naturalIndex, L) % bitstring(aIndex, L);
    }

    return false;
}

bool checkStateVectorPlacement(const SimulatorMPI& sim)
{
    auto iperm = inversePermutation(sim.GetQubitsPermutation());

    bool res = true;
    for(size_t i = 0; i < sim.vec_.size(); ++i) {
        res &= checkAmplitudePlacement(sim.LocalQubitsCount(), sim.world_.rank(), i, sim.vec_[i], sim.GetQubitsPermutation());
    }

    return res;
}

int main(int argc, const char** argv) {

    uint64_t L = 0;
    uint64_t M = 0;
    uint64_t nthreads = 1;
    uint64_t rank = 0;
    std::vector<uint64_t> swap_pairs;
    std::vector<uint64_t> perm_pairs;

    po::options_description desc("Options");
    desc.add_options()
            ("help", "produce help message")
            ("L", po::value<uint64_t>(&L), "total number of qubits")
            ("M", po::value<uint64_t>(&M), "number of local qubits")
            ("nthreads", po::value<uint64_t>(&nthreads), "number of OMP threads")
            ("rank", po::value<uint64_t>(&rank), "rank")
            ("swap", po::value<std::vector<uint64_t>>(&swap_pairs)->multitoken(), "pair to swap")
            ("perm", po::value<std::vector<uint64_t>>(&perm_pairs)->multitoken(), "initial permutation by pairs")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 0;
    }

    SimulatorMPI sim(1, 33, 4);

    DLOG(WARNING) << "simulator created" << endl;

    uint64_t n = 0;

    std::vector<int64_t> ids(L);
    for(size_t id = 0; id < ids.size(); ++id) {
        ids[id] = n++;
    }

    DLOG(WARNING) << "allocating qureg" << endl;
    sim.AllocateQureg( ids );
    DLOG(WARNING) << "qureg allocated" << endl;

//     DLOG(INFO) << endl << endl;
//     DLOG(INFO) << "allocated new state vector";
//     auto sv = allocateData1(sim.world.rank(), sim.M);
//     std::swap(sim.vec_, sv);
//     checkStateVectorPlacement(sim);

//     DLOG(INFO) << endl << endl;
//     DLOG(INFO) << "apply_controlled_gate(I, {0}, {})";
//     sim.apply_controlled_gate(I, {0}, {});
//     sim.run();
//     checkStateVectorPlacement(sim);


//     DLOG(INFO) << endl << endl;
//     DLOG(INFO) << "apply_controlled_gate(I, {0}, {L-1, L-2})";
//     sim.apply_controlled_gate(I, {0}, {L-1, L-2});
//     sim.run();
//     checkStateVectorPlacement(sim);
    hiq::printAmplitudes(sim);

    LOG(INFO) << endl << endl << format("initialize state vector by H^%d") % L;
    std::fill(sim.vec_.begin(), sim.vec_.end(), std::pow(1.0/sqrt2, L));
    hiq::printAmplitudes(sim);

    for( auto id: {L-2} ) {
        LOG(INFO) << endl << endl;
//         DLOG(INFO) << format("apply_controlled_gate(I, {%d}, {%d, %d})") % id % (L-1) % (L-2);
        sim.ApplyGate(RZpi, {id}, {sim.LocalQubitsCount() - 1, L-1});
        sim.Run();
//         checkStateVectorPlacement(sim);
        hiq::printAmplitudes(sim);
    }

    DLOG(WARNING) << "final barrier" << endl;
    sim.world_.barrier();
}
