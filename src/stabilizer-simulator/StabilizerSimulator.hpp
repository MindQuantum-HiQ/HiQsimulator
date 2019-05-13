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

#include <vector>
#include <iostream>
#include <cmath>
#include <random>
#include <functional>
#include <algorithm>
#include "simulator-mpi/alignedallocator.hpp"

#ifdef _MSC_VER
#  include <intrin.h>
#else
#  include <x86intrin.h>
#endif // _MSC_VER


constexpr unsigned max_storage_depth = 10;


using AlignedIntVec = std::vector<uint32_t, aligned_allocator<uint32_t, 256>>;
using VecOfMasks = std::vector<AlignedIntVec>;
struct Gates{
    Gates(unsigned n = 1, unsigned max_d = 1){
        unsigned num_masks = std::ceil(n / 32.);
        unsigned nmasks_8 = 8 * std::ceil(num_masks / 8.);
        H = VecOfMasks(max_d, AlignedIntVec(nmasks_8, 0));
        S = VecOfMasks(max_d, AlignedIntVec(nmasks_8, 0));
        C = std::vector<std::vector<std::pair<unsigned,unsigned>>>(max_d);
        max_d_ = max_d;
        num_gates = 0;
    }
    void reset(){
        for (unsigned d = 0; d < H.size(); ++d){
            for (unsigned i = 0; i < H[d].size(); ++i){
                H[d][i] = 0;
                S[d][i] = 0;
            }
        }
        num_gates = 0;
        C = std::vector<std::vector<std::pair<unsigned,unsigned>>>(max_d_);
    }
    unsigned num_gates;
    unsigned max_d_;
    VecOfMasks H;
    VecOfMasks S;
    std::vector<std::vector<std::pair<unsigned,unsigned>>> C;
};

class StabilizerSimulator{
public:
    StabilizerSimulator(unsigned num_qubits, unsigned seed = 0) : num_qubits_(num_qubits), rnd_eng_(seed){
        // Saves x and z in a vertical order

        unsigned hlen = std::ceil(std::ceil(num_qubits / 32.) / 8.) * 8; // number of 32-bit integers for one row/stabilizer generator
        unsigned vlen = std::ceil((2 * num_qubits + 1) / 32.);
        x = AlignedIntVec(hlen * (2 * num_qubits + 1));
        z = AlignedIntVec(hlen * (2 * num_qubits + 1));
        r = AlignedIntVec(vlen);
        row_length_ = hlen;
        // Init all qubits to state 0
        #pragma omp parallel for schedule(static,32)
        for(unsigned i = 0; i < 2 * num_qubits; ++i){
            for (unsigned j = 0; j < row_length_; ++j)
                z[i * row_length_ + j] = x[i * row_length_ + j] = 0;
            set_phase_bit(i, 0);
            if (i < num_qubits)
                set_stab_x(i, i, true);
            else
                set_stab_z(i, i - num_qubits, true);
        }
        for (unsigned j = 0; j < row_length_; ++j)
            z[(2 * num_qubits) * row_length_ + j] = x[(2 * num_qubits) * row_length_ + j] = 0;
        set_phase_bit(2 * num_qubits, 0);

        std::uniform_int_distribution<int> dist(0,1);
        rng_ = std::bind(dist, std::ref(rnd_eng_));
        gates_ = Gates(num_qubits, max_storage_depth);
        pos_ = std::vector<unsigned>(num_qubits, 0);
        //std::cout << "Tableau (" << num_qubits << "): " << hlen << " x " << vlen << " with Hsize=" << gates_.H[0].size() << "\n";
    }

    StabilizerSimulator() : rnd_eng_(0){}

    ~StabilizerSimulator() {}

    void AllocateQureg(unsigned num_qubits){
        // Saves x and z in a vertical order
        num_qubits_ = num_qubits;
        unsigned hlen = std::ceil(std::ceil(num_qubits / 32.) / 8.) * 8; // number of 32-bit integers for one row/stabilizer generator
        unsigned vlen = std::ceil((2 * num_qubits + 1) / 32.);
        x = AlignedIntVec(hlen * (2 * num_qubits + 1));
        z = AlignedIntVec(hlen * (2 * num_qubits + 1));
        r = AlignedIntVec(vlen);
        row_length_ = hlen;
        // Init all qubits to state 0
        #pragma omp parallel for schedule(static,32)
        for(unsigned i = 0; i < 2 * num_qubits; ++i){
            for (unsigned j = 0; j < row_length_; ++j)
                z[i * row_length_ + j] = x[i * row_length_ + j] = 0;
            set_phase_bit(i, 0);
            if (i < num_qubits)
                set_stab_x(i, i, true);
            else
                set_stab_z(i, i - num_qubits, true);
        }
        for (unsigned j = 0; j < row_length_; ++j)
            z[(2 * num_qubits) * row_length_ + j] = x[(2 * num_qubits) * row_length_ + j] = 0;
        set_phase_bit(2 * num_qubits, 0);

        std::uniform_int_distribution<int> dist(0,1);
        rng_ = std::bind(dist, std::ref(rnd_eng_));
        gates_ = Gates(num_qubits, max_storage_depth);
        pos_ = std::vector<unsigned>(num_qubits, 0);
        //std::cout << "Tableau (" << num_qubits << "): " << hlen << " x " << vlen << " with Hsize=" << gates_.H[0].size() << "\n";
    }

    void h(unsigned qubit){
        unsigned mask_num = qubit / 32;
        gates_.H[pos_[qubit]][mask_num] |= (1 << (qubit - mask_num * 32));
        pos_[qubit]++;
        gates_.num_gates++;
        if (pos_[qubit] >= max_storage_depth)
            sync();
    }

    inline void h_and_s_kernel(AlignedIntVec const& Hmasks, AlignedIntVec const& Smasks, unsigned stab){
        auto phase_sum = _mm256_setzero_si256();
        for (unsigned i = 0; i < Smasks.size(); i += 8){
            auto hmask = _mm256_load_si256((__m256i*)&Hmasks[i]);
            auto smask = _mm256_load_si256((__m256i*)&Smasks[i]);
            auto xstab = _mm256_load_si256((__m256i*)&x[i + row_length_ * stab]);
            auto zstab = _mm256_load_si256((__m256i*)&z[i + row_length_ * stab]);
            auto smasked_x = _mm256_and_si256(xstab, smask);
            auto smasked_x_and_z = _mm256_and_si256(zstab, smasked_x);
            // keep track of phase for s-gate
            phase_sum = _mm256_xor_si256(smasked_x_and_z, phase_sum);
            zstab = _mm256_xor_si256(smasked_x, zstab); // update zstab for s-gate

            auto hmasked_z = _mm256_and_si256(zstab, hmask);
            auto hmasked_x_and_z = _mm256_and_si256(hmasked_z, xstab);
            phase_sum = _mm256_xor_si256(phase_sum, hmasked_x_and_z); // update phase sum for H-gate
            // apply H-gate
            xstab = _mm256_xor_si256(zstab, xstab);
            auto hmasked_x = _mm256_and_si256(xstab, hmask);
            zstab = _mm256_xor_si256(hmasked_x, zstab);
            xstab = _mm256_xor_si256(zstab, xstab);

            _mm256_store_si256((__m256i*)&x[i + row_length_ * stab], xstab);
            _mm256_store_si256((__m256i*)&z[i + row_length_ * stab], zstab);
        }
        // reduce & store phase sum (for s-gate)
        alignas(32) uint64_t tmpvec[4];
        _mm256_store_si256((__m256i*)&tmpvec, phase_sum);
        uint64_t r1 = tmpvec[0]^tmpvec[1];
        uint64_t r2 = tmpvec[2]^tmpvec[3];
        uint32_t mask = -1;
        uint64_t red1 = r1^r2;
        uint32_t red2 = ((red1 >> 32)&mask) ^ (red1&mask);
        set_phase_bit(stab, get_phase_bit(stab) ^ (_mm_popcnt_u32(red2) & 1));
    }

    void s(unsigned qubit){
        unsigned mask_num = qubit / 32;
        gates_.S[pos_[qubit]][mask_num] |= (1 << (qubit - mask_num * 32));
        pos_[qubit]++;
        gates_.num_gates++;
        if (pos_[qubit] >= max_storage_depth)
            sync();
    }

    // qubit1 is control qubit, qubit2 is target qubit
    void cnot(unsigned qubit1, unsigned qubit2){
        unsigned pos = std::max(pos_[qubit1], pos_[qubit2]);
        gates_.C[pos].push_back(std::make_pair(qubit1, qubit2));
        pos++;
        pos_[qubit1] = pos;
        pos_[qubit2] = pos;
        gates_.num_gates++;
        if (pos_[qubit1] >= max_storage_depth)
            sync();
    }

    inline void cnot_kernel(unsigned qubit1, unsigned qubit2, unsigned stab){
        //std::cout << "kernel with " << qubit1 << ", " << qubit2 << ", " << stab << "\n";
        // 7 int ops and 8 * 4bytes rw
        bool z1 = get_stab_z(stab, qubit1);
        bool z2 = get_stab_z(stab, qubit2);
        toggle_stab_z(stab, qubit1, z2);
        bool x1 = get_stab_x(stab, qubit1);
        bool x2 = get_stab_x(stab, qubit2);
        auto tmp = x1 & z2 & (x2 == z1);
        set_phase_bit(stab, get_phase_bit(stab) ^ tmp);
        set_stab_x(stab, qubit2, x2 ^ x1);
        //set_stab_z(stab, qubit1, z1 ^ z2);
        //toggle_stab_x(stab, qubit2, get_stab_x(stab, qubit1));

    }

    void sync(){
        if (gates_.num_gates == 0)
            return;
        #pragma omp parallel for schedule(static,32)
        for(unsigned stab = 0; stab < 2 * num_qubits_; ++stab){
            for (unsigned d = 0; d < gates_.H.size(); ++d){
                h_and_s_kernel(gates_.H[d], gates_.S[d], stab);
                for (auto const& cnot_tuple : gates_.C[d])
                    cnot_kernel(cnot_tuple.first, cnot_tuple.second, stab);
            }
        }
        gates_.reset();
        for (auto &p : pos_)
            p = 0;
    }

    unsigned compute_phase(unsigned stabilizer_h, unsigned stabilizer_i) const{
        // only inner loop: 22 int ops and 4 * 4byte rw

        // phase is i^exponent, where exponent is saved as a 2 bit number
        auto exp_low = _mm256_setzero_si256();
        auto exp_high1 = _mm256_setzero_si256();
        auto exp_high2 = _mm256_setzero_si256();
        unsigned start_index_h = stabilizer_h * row_length_;
        unsigned start_index_i = stabilizer_i * row_length_;
        auto ones = _mm256_set1_epi32((uint32_t)((1ULL << 32) - 1));
        for (unsigned i = 0; i < row_length_; i += 8){
            auto xh = _mm256_load_si256((__m256i*)&x[start_index_h + i]);
            auto xi = _mm256_load_si256((__m256i*)&x[start_index_i + i]);
            auto zh = _mm256_load_si256((__m256i*)&z[start_index_h + i]);
            auto zi = _mm256_load_si256((__m256i*)&z[start_index_i + i]);
            auto not_xh = _mm256_xor_si256(ones, xh);
            auto not_zi = _mm256_xor_si256(ones, zi);
            auto q = _mm256_and_si256(_mm256_and_si256(_mm256_or_si256(xi, zh), _mm256_or_si256(not_xh, zi)),
                                      _mm256_or_si256(not_zi, xh));
            auto r = _mm256_and_si256(_mm256_or_si256(xi, zi), _mm256_or_si256(xh, zh));
            auto s = _mm256_or_si256(_mm256_xor_si256(xi, xh), _mm256_xor_si256(zi, zh));
            auto r_and_s = _mm256_and_si256(r, s);
            exp_high1 = _mm256_xor_si256(_mm256_and_si256(exp_low, r_and_s), exp_high1);
            exp_high2 = _mm256_xor_si256(exp_high2, _mm256_and_si256(q, r_and_s));
            exp_low = _mm256_xor_si256(exp_low, r_and_s);
            /*
            uint32_t stab_h_x = x[start_index_h + i];
            uint32_t stab_h_z = z[start_index_h + i];
            uint32_t stab_i_x = x[start_index_i + i];
            uint32_t stab_i_z = z[start_index_i + i];

            uint32_t q = (stab_i_x | stab_h_z) & (stab_i_z | ~stab_h_x) & (~stab_i_z | stab_h_x);
            uint32_t r = (stab_i_x | stab_i_z) & (stab_h_x | stab_h_z);
            uint32_t s = ~(~(stab_i_x ^ stab_h_x) & ~(stab_i_z ^ stab_h_z));
            uint32_t r_and_s = r & s;
            exp_high = (exp_low & r_and_s) ^ (exp_high ^ (q & r_and_s));
            exp_low ^= r_and_s;
            */
        }
        auto exp_high = _mm256_xor_si256(exp_high1, exp_high2);
        alignas(32) uint64_t eh[4], el[4];
        _mm256_store_si256((__m256i*)&eh, exp_high);
        _mm256_store_si256((__m256i*)&el, exp_low);

        eh[0] ^= eh[1];
        eh[2] ^= eh[3];
        el[0] ^= el[1];
        el[2] ^= el[3];

        eh[0] ^= eh[2];
        el[0] ^= el[2];
        uint32_t mask = (1ULL << 32) - 1;
        uint32_t eh_sm = ((eh[0] >> 32)&mask) ^ (eh[0]&mask);
        uint32_t el_sm = ((el[0] >> 32)&mask) ^ (el[0]&mask);
        int exponent = 2 * _mm_popcnt_u32(eh_sm) + _mm_popcnt_u32(el_sm);
        // compute positive modulo.
        unsigned int phase = (((exponent + 2 * get_phase_bit(stabilizer_h) +
                                2 * get_phase_bit(stabilizer_i)) % 4) + 4) % 4;
        return phase;
    }


    // Left-multiply stabilizer generator h by stabilizer generator i,
    // i.e., h <- i * h
    // For all qubits j: x_hj ^= x_ij and z_hj ^= z_ij
    void rowsum(unsigned stabilizer_h, unsigned stabilizer_i){
        // phase is either 0 (+1) or 2 (-1) which we map to 0 (+1) and 1 (-1) as before:
        set_phase_bit(stabilizer_h, compute_phase(stabilizer_h, stabilizer_i)/2);
        unsigned start_index_h = stabilizer_h * row_length_;
        unsigned start_index_i = stabilizer_i * row_length_;

        for (unsigned i = 0; i < row_length_; i += 8){
            // x_hj ^= x_ij
            auto xh = _mm256_load_si256((__m256i*)&x[start_index_h + i]);
            auto xi = _mm256_load_si256((__m256i*)&x[start_index_i + i]);
            _mm256_store_si256((__m256i*)&x[start_index_h + i], _mm256_xor_si256(xh, xi));
            //x[start_index_h + i] ^= x[start_index_i + i];
            // same for z_hj:
            auto zh = _mm256_load_si256((__m256i*)&z[start_index_h + i]);
            auto zi = _mm256_load_si256((__m256i*)&z[start_index_i + i]);
            _mm256_store_si256((__m256i*)&z[start_index_h + i], _mm256_xor_si256(zh, zi));
            //z[start_index_h + i] ^= z[start_index_i + i];
        }
    }


    int measure(unsigned qubit){
        sync();
        bool option = false;
        unsigned p;
        // Check if there is a x_{p, qubit} = 1 for p in num_qubits, ..., 2 * num_qubits -1
        for(unsigned i = num_qubits_; i < 2 * num_qubits_; ++i){
            if(get_stab_x(i, qubit)){
                option = true;
                p = i;
                break;
            }
        }
        if(option){  // Case I (random outcome)
            #pragma omp parallel for schedule(static,32)
            for(unsigned j = 0; j < p; ++j){
                if(get_stab_x(j, qubit)){
                    rowsum(j, p);
                }
            }
            #pragma omp parallel for schedule(static,32)
            for (unsigned j = p+1; j < 2 * num_qubits_; ++j){
                if(get_stab_x(j, qubit)){
                    rowsum(j, p);
                }
            }
            // set row p-num_qubits equal to row p
            // set row p equal to 0 except z_{p,qubit} = 1 and r_p 0 or 1 with equal probability
            unsigned start_p_m_num_qubits = get_index(p-num_qubits_, 0);
            unsigned start_p = get_index(p, 0);
            for(unsigned k = 0; k < row_length_; ++k){
                x[start_p_m_num_qubits + k] = x[start_p + k];
                x[start_p + k] = 0;
                z[start_p_m_num_qubits + k] = z[start_p + k];
                z[start_p + k] = 0;
            }
            set_stab_z(p, qubit, 1);
            set_phase_bit(p-num_qubits_, get_phase_bit(p));
            bool measurement = rng_();
            set_phase_bit(p, measurement);
            return measurement;
        }
        else{  // Case II (deterministic outcome)
            // set row 2*num_qubits equal to 0 (this was a previously unused row)
            unsigned start = get_index(2*num_qubits_,0);
            for(unsigned k = 0; k < row_length_; ++k){
                x[start+k] = 0;
                z[start+k] = 0;
            }
            set_phase_bit(2*num_qubits_, 0);
            // rowsum(2*num_qubits, i + num_qubits) for all i in 0,...,num_qubits-1 (i.e. a destabilizer)
            // for which x_{i,qubit} = 1
            for(unsigned l = 0; l < num_qubits_; ++l){
                if(get_stab_x(l, qubit)){
                    rowsum(2*num_qubits_, l + num_qubits_);
                }
            }
            return get_phase_bit(2*num_qubits_);
        }

    }


    friend std::ostream& operator<<(std::ostream& out, StabilizerSimulator const& sim);

    // For testing:
    friend void test_cnot();
    friend void test_h();
    friend void test_s();
    friend void test_compute_phase_and_rowsum();
    friend void test_random_measurement();

private:
    // index for x, z
    unsigned get_index(unsigned stab, unsigned qubit) const{
        return stab * row_length_ + (qubit >> 5);
    }

    bool get_stab_x(unsigned stab, unsigned qubit) const{
        return (x[get_index(stab, qubit)] >> (qubit & 31)) & 1;
    }

    bool get_stab_z(unsigned stab, unsigned qubit) const{
        return (z[get_index(stab, qubit)] >> (qubit & 31)) & 1;
    }

    bool get_phase_bit(unsigned stab) const{
        return (r[stab >> 5] >> (stab & 31)) & 1;
    }

    // used for storage independent testing
    void toggle_stab_x(unsigned stab, unsigned qubit, unsigned long long one = 1){
        x[get_index(stab, qubit)] ^= (one << (qubit & 31));
    }

    void set_stab_x(unsigned stab, unsigned qubit, bool value){
        if (get_stab_x(stab, qubit) != value)
            toggle_stab_x(stab, qubit);
    }

    void toggle_stab_z(unsigned stab, unsigned qubit, unsigned long long one = 1){
        z[get_index(stab, qubit)] ^= (one << (qubit & 31));
    }

    void set_stab_z(unsigned stab, unsigned qubit, bool value){
        if (get_stab_z(stab, qubit) != value)
            toggle_stab_z(stab, qubit);
    }

    void toggle_phase_bit(unsigned stab, unsigned long long one = 1){
        r[stab>>5] ^= one << (stab & 31);
    }

    void set_phase_bit(unsigned stab, bool value){
        if (get_phase_bit(stab) != value)
            toggle_phase_bit(stab);
    }

    std::vector<uint32_t, aligned_allocator<uint32_t, 256>> x; // len is N x 2N+1
    std::vector<uint32_t, aligned_allocator<uint32_t, 256>> z; // len is N x 2N+1
    std::vector<uint32_t, aligned_allocator<uint32_t, 256>> r; // len is 2N+1

    unsigned num_qubits_;
    unsigned row_length_;

    std::mt19937 rnd_eng_;
    std::function<int()> rng_;

    Gates gates_;
    std::vector<unsigned> pos_;
};


std::ostream& operator<<(std::ostream& out, StabilizerSimulator const& sim){
    for (unsigned stabilizer = 0; stabilizer < 2 * sim.num_qubits_ + 1; ++stabilizer){
        for (unsigned qubit = 0; qubit < sim.num_qubits_; ++qubit){
            out << sim.get_stab_x(stabilizer, qubit) << " ";
        }
        for (unsigned qubit = 0; qubit < sim.num_qubits_; ++qubit){
            out << sim.get_stab_z(stabilizer, qubit) << " ";
        }
        out << "| " << sim.get_phase_bit(stabilizer) << "\n";
    }
    return out;
}
