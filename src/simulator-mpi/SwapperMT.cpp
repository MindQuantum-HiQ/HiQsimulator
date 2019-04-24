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

#include "SwapperMT.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/format.hpp>
#include "mpi_ext.hpp"
#include <mpi.h>
#include <glog/logging.h>

#include "swapping.hpp"

using boost::sync_queue_is_closed;
using boost::format;

const size_t SwapperMT::MaxGlobal = 17;

void f_producer(SwapperMT& s, const std::vector<uint64_t>& swap_bits) {
    size_t start_free_idx = 0;
    while(start_free_idx < (1ul << (s.M - s.n_bits)) ) {
        auto arrs = s.buffs.old_arrays.pull_front();

        start_free_idx =
            swapping::Swapping<SwapperMT::MaxGlobal>::calcSwap( s.n_bits, s.state_vector.data(), s.n, start_free_idx
                                                              , arrs->svalues.data(), arrs->indices.data(), swap_bits);

        s.buffs.fresh_arrays.push_back(arrs);
    }

    s.buffs.fresh_arrays.push_back(nullptr);

    DLOG(INFO) << "f_producer(): exit";
}

void f_consumer2(SwapperMT& s, const mpi::communicator& comm) {
    do {
        SwapperMT::swap_buffers_type::swap_arrays_type* arrs;
        s.buffs.fresh_arrays2.pull_front(arrs);
        if( arrs == nullptr )
            break;

        size_t comm_size = s.comm_size;
        size_t n = s.n;

        uint64_t comm_rank = comm.rank();
        size_t c = 0;
        while( c < comm_rank ) {
            for( size_t i = 0; i < n; ++i ) {
                size_t ii = n*c + i;
                size_t idx = arrs->indices[ii];
                s.state_vector[idx] = arrs->rvalues[ii];
            }
            ++c;
        }

        // skip my rank
        ++c;
        while( c < comm_size ) {
            for( size_t i = 0; i < n; ++i ) {
                size_t ii = n*c + i;
                size_t idx = arrs->indices[ii];
                s.state_vector[idx] = arrs->rvalues[ii];
            }
            ++c;
        }


        s.buffs.old_arrays.push_back(arrs);

    } while(true);

    DLOG(INFO) << "f_consumer2(): exit";
}

void SwapperMT::runProducer(const std::vector<uint64_t>& aSwap_bits) {
    this->producer = std::thread(f_producer, std::ref(*this), std::ref(aSwap_bits));
}

void SwapperMT::runConsumer2(const mpi::communicator& comm) {
    this->consumer2 = std::thread(f_consumer2, std::ref(*this), std::ref(comm));
}

void SwapperMT::doSwap(int rank, const mpi::communicator& comm, uint64_t color, const std::vector<uint64_t>& aSwap_bits) {

    DLOG(INFO) << format("M: %d, rank: %d, color: %d, swap_bits: %s") % M % rank % color % print(aSwap_bits);

    this->runConsumer2(comm);
    this->runProducer(aSwap_bits);

    do {
        swap_buffers_type::swap_arrays_type* arrs;
        buffs.fresh_arrays.pull_front(arrs);
        if( arrs == nullptr)
            break;

        mpi::all_to_all(comm, arrs->svalues.data(), n, arrs->rvalues.data());

        buffs.fresh_arrays2.push_back(arrs);

    } while(true);

    buffs.fresh_arrays2.push_back(nullptr);

    this->join();

    DLOG(INFO) << "doSwap(): exit";
}
