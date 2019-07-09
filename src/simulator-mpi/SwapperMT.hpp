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

#ifndef SWAPPERMT_HPP
#define SWAPPERMT_HPP

#include <glog/logging.h>

#include <atomic>
#include <boost/format.hpp>
#include <boost/mpi.hpp>
#include <complex>
#include <list>
#include <thread>
#include <vector>

#include "simulator-mpi/SimulatorMPI.hpp"
#include "simulator-mpi/SwapArrays.hpp"

class EXPORT_API SwapperMT
{
public:
     typedef std::complex<double> value_type;
     typedef SwapBuffers<value_type> swap_buffers_type;

     static const uint64_t MaxGlobal;

     mpi::communicator world;

     SimulatorMPI::StateVector& state_vector;
     const uint64_t rank;
     const uint64_t M;
     const size_t comm_size;

     uint64_t n;

     swap_buffers_type& buffs;

     uint64_t n_bits;

     SwapperMT(mpi::communicator& aWorld,
               SimulatorMPI::StateVector& aStateVector, size_t aRank,
               uint64_t aM, size_t aComm_size, swap_buffers_type& aBuffs,
               uint64_t nBits)
         : world(aWorld),
           state_vector(aStateVector),
           rank(aRank),
           M(aM),
           comm_size(aComm_size),
           n(calcSendCount(comm_size, M, aBuffs.size())),
           buffs(aBuffs),
           n_bits(nBits)
     {
          DLOG(INFO) << boost::format("SwapperMT(): n: %d, comm_size: %d") % n
                            % comm_size;
     }

     ~SwapperMT()
     {}

     static uint64_t calcSendCount(size_t comm_size, uint64_t M,
                                   uint64_t maxSendBytes)
     {
          size_t state_vector_size = (1ul << M);

          uint64_t n0 = ((maxSendBytes / sizeof(value_type)) / comm_size);
          n0 = n0 < 1 ? 1 : n0;
          uint64_t n = (state_vector_size / comm_size) < n0
                           ? (state_vector_size / comm_size)
                           : n0;

          return n;
     }

     void runProducer(const std::vector<uint64_t>& aSwap_bits);
     void runConsumer2(const mpi::communicator& comm);

     void join()
     {
          if (this->producer.joinable())
               this->producer.join();

          if (this->consumer2.joinable())
               this->consumer2.join();
     }

     void doSwap(int rank, const mpi::communicator& comm, uint64_t color,
                 const std::vector<uint64_t>& aSwap_bits);

private:
     std::thread producer;
     std::thread consumer2;
};

#endif  // SWAPPERMT_HPP
