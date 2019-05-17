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

#include "simulator-mpi/SimulatorMPI.hpp"

#include <glog/logging.h>

#include <cmath>
#ifdef _OPENMP
#     include <omp.h>
#endif  // _OPENMP
#include "SwapperMT.hpp"
#include "funcs.hpp"
#include "mpi_ext.hpp"

constexpr size_t SimulatorMPI::kNotFound_;

class GlogSingleton
{
public:
     static GlogSingleton &instance()
     {
          static GlogSingleton qqq;
          return qqq;
     }

private:
     GlogSingleton()
     {
          google::InitGoogleLogging("SimulatorMPI");
     }

     ~GlogSingleton()
     {
          google::ShutdownGoogleLogging();
     }
};

SimulatorMPI::SimulatorMPI(uint64_t seed, size_t max_local,
                           size_t max_cluster_size)
    : SimulatorMPI(mpi::communicator(), seed, max_local, max_cluster_size)
{}

SimulatorMPI::SimulatorMPI(mpi::communicator aWorld, uint64_t seed,
                           size_t max_local, size_t max_cluster_size)
    : env_(boost::mpi::threading::level::funneled),
      world_(aWorld),
      kMaxFloatError_(1e-12),
      kMinLocal_(max_cluster_size),
      kMaxLocal_(max_local),
      kMaxGlobal_(static_cast<int>(log2(world_.size()))),
      kMaxClusterSize_(max_cluster_size),
      globals_(static_cast<Index>(kMaxGlobal_), kNotFound_),
      rank_(world_.rank()),
      buffs_(4)
{
     start_time = Clock::now();
     GlogSingleton::instance();

     mpi::broadcast(world_, seed, 0);
     rnd_eng_ = RndEngine(seed);

     VLOG(0) << boost::format(
                    "ctor(): rank = %d; seed = %u; max_local = %d; "
                    "max_cluster_size = %d") %
                    rank_ % seed % max_local % max_cluster_size;

     vec_.reserve(1ul << kMaxLocal_);
     vec_.resize(1);
     if (rank_ == 0)
          vec_[0] = 1.;  // all-zero initial state

     std::uniform_real_distribution<double> dist(0., 1.);
     rng_ = std::bind(dist, std::ref(rnd_eng_));

     StartStage();
}

SimulatorMPI::~SimulatorMPI()
{
     EndStage();

     auto total_duration = Duration(Clock::now() - start_time).count();
     VLOG(0) << boost::format("     *** TOTAL STATS ***     ");
     VLOG(0) << boost::format(" Time                   %.3lf") % total_duration;
     VLOG(0) << boost::format("   -- Runs              %.3lf") %
                    total_runs_duration;
     VLOG(0) << boost::format("   -- Swaps             %.3lf") %
                    total_swap_duration;
     VLOG(0) << boost::format("   -- Measures          %.3lf") %
                    total_measure_duration;
     VLOG(0) << boost::format("   -- Allocations       %.3lf") %
                    total_alloc_duration;
     VLOG(0) << boost::format("   -- Deallocations     %.3lf") %
                    total_dealloc_duration;
     VLOG(0) << boost::format(" Gates                  %d") % total_gates;
     VLOG(0) << boost::format(" Runs                   %d") % total_runs;
     VLOG(0) << boost::format(" Stages                 %d") % total_stages;
     VLOG(0) << boost::format(" Gates/run              %.3lf") %
                    (static_cast<Float>(total_gates) / total_runs);
     VLOG(0) << boost::format(" Time/run               %.3lf") %
                    (total_runs_duration / total_runs);
     VLOG(0) << boost::format(" Gates/stage            %.3lf") %
                    (static_cast<Float>(total_gates) / total_stages);
     VLOG(0) << boost::format(" Runs/stage             %.3lf") %
                    (static_cast<Float>(total_runs) / total_stages);
}

template <class V>
inline void FillVector(typename V::iterator start, typename V::iterator end,
                       const typename V::value_type v)
{
     uint64_t sz = end - start;

#pragma omp parallel for schedule(static)
     for (size_t i = 0; i < sz; ++i) {
          start[i] = v;
     }
}

inline void SimulatorMPI::CheckNorm()
{
     Float local_norm = norm(vec_.begin(), vec_.end());

     Float norm = 0.0;
     mpi::all_reduce(world_, local_norm, norm, std::plus<Float>());

     VLOG(4) << boost::format("CheckNorm(): norm = %.3lf; local_norm = %.3lf") %
                    norm % local_norm;
     VLOG(4) << boost::format("CheckNorm(): local state vector: ")
             << print(vec_);

     if (std::abs(norm - 1) > kMaxFloatError_) {
          auto message = "CheckNorm(): state vector norm is invalid";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }
}

void SimulatorMPI::AllocateLocalQubit(Index id)
{
     VLOG(1) << boost::format("AllocateLocalQubit(): id = %u; bit = %u") % id %
                    locals_.size();
     locals_.push_back(id);
     vec_.resize(vec_.size() * 2);
}

void SimulatorMPI::AllocateGlobalQubit(Index id)
{
     auto pos = ArrayFindSure(globals_, kNotFound_);
     VLOG(1) << boost::format("AllocateGlobalQubit(): id = %u; bit = %u") % id %
                    pos;
     globals_[pos] = id;
}

void SimulatorMPI::AllocateQubit(Index id)
{
     auto start_alloc_time = Clock::now();
     VLOG(1) << boost::format("AllocateQubit(): id = %u") % id;

     auto local_size = locals_.size();

     if (local_size < kMinLocal_) {
          VLOG(1) << "AllocateQubit(): local qubit (too few local qubits)";
          AllocateLocalQubit(id);
     }
     else if (ArrayFind(globals_, kNotFound_) != kNotFound_) {
          VLOG(1) << "AllocateQubit(): global qubit (too few global qubits)";
          AllocateGlobalQubit(id);
     }
     else if (local_size < kMaxLocal_) {
          VLOG(1) << "AllocateQubit(): local qubit (default case)";
          AllocateLocalQubit(id);
     }
     else {
          auto message =
              (boost::format(
                   "AllocateQubit(): can't allocate more than %u qubits") %
               (globals_.size() + locals_.size()))
                  .str();
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     auto alloc_duration = Duration(Clock::now() - start_alloc_time).count();
     total_alloc_duration += alloc_duration;
}

void SimulatorMPI::AllocateQureg(const std::vector<Index> &ids, Complex init)
{
     VLOG(1) << boost::format("AllocateQureg(): ids = ") << print(ids);
     VLOG(1) << boost::format("AllocateQureg(): init = %f") % init;

     if (init != 0.0 && !locals_.empty()) {
          auto message =
              "AllocateQureg(): initialization of only first qureg is "
              "supported";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     for (auto i: ids) {
          AllocateQubit(i);
     }

     if (init != 0.0) {
          uint64_t global_msk = 0;

          for (size_t pos = 0; pos < globals_.size(); ++pos) {
               if ((size_t) globals_[pos] != kNotFound_)
                    global_msk |= 1ul << pos;
          }

          if ((rank_ & ~global_msk) == 0)
               FillVector<StateVector>(vec_.begin(), vec_.end(), init);
     }

#ifndef NDEBUG
     CheckNorm();
#endif
}

size_t SimulatorMPI::ArrayFind(const std::vector<Index> &v, Index val) const
{
     size_t pos = find(v.begin(), v.end(), val) - v.begin();
     if (pos == v.size()) {
          return kNotFound_;
     }
     else {
          return pos;
     }
}

size_t SimulatorMPI::ArrayFindSure(const std::vector<Index> &v, Index val) const
{
     auto pos = ArrayFind(v, val);
     if (pos == kNotFound_) {
          std::ostringstream message;
          message << boost::format("ArrayFindSure(): Can't find %u in ") % val
                  << print(v);
          LOG(ERROR) << message.str();
          world_.barrier();
          throw std::runtime_error(message.str());
     }
     return pos;
}

void SimulatorMPI::DeallocateLocalQubit(Index id)
{  // TODO make parallel
     VLOG(1) << boost::format("DeallocateLocalQubit(): id = %u") % id;

     Float sum[] = {0.0, 0.0};
     auto pos = ArrayFindSure(locals_, id);
     for (size_t i = 0; i < vec_.size(); ++i) {
          sum[i >> pos & 1] += std::norm(vec_[i]);
     }

     Float all[] = {0.0, 0.0};
     mpi::all_reduce(world_, sum, 2, all, std::plus<Float>());

     VLOG(1)
         << boost::format(
                "DeallocateLocalQubit(): norm(0) = %.3lf; norm(1) = %.3lf") %
                all[0] % all[1];
     if (!((all[0] > kMaxFloatError_) ^ (all[1] > kMaxFloatError_))) {
          auto message =
              (boost::format("DeallocateLocalQubit(): qubit %u is entangled") %
               id)
                  .str();
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     if (all[0] > kMaxFloatError_) {
          for (size_t i = (1ul << (pos)); (i << 1) < vec_.size();
               i += (1ul << (pos))) {
               for (size_t j = 0; j < (1ul << pos); ++j) {
                    vec_[i + j] = vec_[(i << 1) + j];
               }
          }
     }
     else {
          for (size_t i = 0; i < vec_.size(); i += (1ul << (pos + 1))) {
               for (size_t j = 0; j < (1ul << pos); ++j) {
                    vec_[(i >> 1) + j] = vec_[i + j + (1ul << (pos))];
               }
          }
     }

     locals_.erase(locals_.begin() + pos);
     vec_.resize(vec_.size() / 2);
}

void SimulatorMPI::DeallocateGlobalQubit(Index id)
{  // TODO make parallel
     VLOG(1) << boost::format("DeallocateGlobalQubit(): id = %u") % id;

     Float sum[] = {0.0, 0.0};
     auto pos = ArrayFindSure(globals_, id);
     for (size_t i = 0; i < vec_.size(); ++i) {
          sum[rank_ >> pos & 1] += std::norm(vec_[i]);
     }

     Float all[] = {0.0, 0.0};
     mpi::all_reduce(world_, sum, 2, all, std::plus<Float>());

     VLOG(1)
         << boost::format(
                "DeallocateGlobalQubit(): norm(0) = %.3lf; norm(1) = %.3lf") %
                all[0] % all[1];
     if (!((all[0] > kMaxFloatError_) ^ (all[1] > kMaxFloatError_))) {
          auto message =
              (boost::format("DeallocateGlobalQubit(): qubit %u is entangled") %
               id)
                  .str();
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     if (all[1] > kMaxFloatError_) {
          VLOG(1) << boost::format(
                         "DeallocateGlobalQubit(): deallocating qubit %u in "
                         "|1> state") %
                         id;
          auto local_qubit = locals_.back();
          SwapQubits({id, local_qubit});
          Matrix x_gate = {{Complex{0}, Complex{1}}, {Complex{1}, Complex{0}}};
          ApplyGate(x_gate, {id}, {});
          Run();
          SwapQubits({local_qubit, id});
     }

     globals_[pos] = static_cast<Index>(-1);

     VLOG(2) << boost::format("DeallocateGlobalQubit(): qubit %u deallocated") %
                    id;
}

void SimulatorMPI::DeallocateQubit(Index id)
{
     auto start_dealloc_time = Clock::now();
     VLOG(1) << boost::format("DeallocateQubit(): id = %u") % id;

     auto local_size = locals_.size();
     auto global_size =
         globals_.size() - std::count(globals_.begin(), globals_.end(), -1);

     if (ArrayFind(locals_, id) != kNotFound_) {
          VLOG(1) << boost::format("DeallocateQubit(): qubit %u is local now") %
                         id;

          if (local_size > kMinLocal_ || global_size == 0) {
               VLOG(1) << "DeallocateQubit(): just deallocate this local qubit";
               DeallocateLocalQubit(id);
          }
          else {
               size_t i = std::find_if(globals_.begin(), globals_.end(),
                                       [](Index id) {
                                            return (size_t) id != kNotFound_;
                                       }) -
                          globals_.begin();
               VLOG(1)
                   << boost::format(
                          "DeallocateQubit(): swapping with global qubit %u") %
                          globals_[i];
               SwapQubits({globals_[i], id});
               DeallocateGlobalQubit(id);
          }
     }
     else {
          VLOG(1) << boost::format(
                         "DeallocateQubit(): qubit %u is global qubit now") %
                         id;

          if (local_size > kMinLocal_ || global_size == 0) {
               VLOG(1)
                   << boost::format(
                          "DeallocateQubit(): swapping with local qubit %u") %
                          locals_.back();
               SwapQubits({id, locals_.back()});
               DeallocateLocalQubit(id);
          }
          else {
               VLOG(1)
                   << "DeallocateQubit(): just deallocate this global qubit";
               DeallocateGlobalQubit(id);
          }
     }

#ifndef NDEBUG
     CheckNorm();
#endif

     auto dealloc_duration =
         Duration(Clock::now() - start_dealloc_time).count();
     total_dealloc_duration += dealloc_duration;
}

uint64_t SimulatorMPI::IdsToBits(const std::vector<Index> &ids,
                                 const std::vector<Index> &perm) const
{
     uint64_t mask = 0;
     for (auto i: ids) {
          size_t pos = ArrayFind(perm, i);
          if (pos != kNotFound_) {
               mask |= (1ul << pos);
          }
     }
     return mask;
}

void SimulatorMPI::Run()
{
     auto start_run_time = Clock::now();

     Matrix m;
     std::vector<Index> ids, ctrls;

     uint64_t flags = 0;
     fused_gates_.perform_fusion(m, ids, ctrls, flags);

     VLOG(1) << "Run(): ids = " << print(ids);
     VLOG(1) << "Run(): ctrls = " << print(ctrls);
     VLOG(3) << "Run(): globals = " << print(globals_);
     VLOG(3) << "Run(): locals = " << print(locals_);
     VLOG(4) << "Run(): matrix = ";
     for (auto &row: m) {
          VLOG(4) << print(row);
     }

     bool diag = static_cast<bool>(flags & MatProps::IS_DIAG);
     VLOG(2) << "Run(): is_diagonal = " << diag;

     auto ctrl_mask = IdsToBits(ctrls, locals_);

     std::vector<uint32_t> ids_pos(ids.size());
     for (size_t i = 0; i < ids.size(); ++i) {
          ids_pos[i] = static_cast<uint32_t>(ArrayFindSure(locals_, ids[i]));
     }

     switch (ids.size()) {
          case 0:
               if (m[0][0] != static_cast<StateVector::value_type>(1)) {
                    kernelK_diag1<StateVector, StateVector::value_type>(
                        vec_, m[0][0]);
               }
               break;
          case 1:
#pragma omp parallel
               diag ? kernelK<StateVector, Matrix, kernel_core_diag>(
                          vec_, ids_pos[0], m, ctrl_mask)
                    : kernelK<StateVector, Matrix, kernel_core>(
                          vec_, ids_pos[0], m, ctrl_mask);
               break;
          case 2:
#pragma omp parallel
               diag ? kernelK<StateVector, Matrix, kernel_core_diag>(
                          vec_, ids_pos[1], ids_pos[0], m, ctrl_mask)
                    : kernelK<StateVector, Matrix, kernel_core>(
                          vec_, ids_pos[1], ids_pos[0], m, ctrl_mask);
               break;
          case 3:
#pragma omp parallel
               diag
                   ? kernelK<StateVector, Matrix, kernel_core_diag>(
                         vec_, ids_pos[2], ids_pos[1], ids_pos[0], m, ctrl_mask)
                   : kernelK<StateVector, Matrix, kernel_core>(
                         vec_, ids_pos[2], ids_pos[1], ids_pos[0], m,
                         ctrl_mask);
               break;
          case 4:
#pragma omp parallel
               diag ? kernelK<StateVector, Matrix, kernel_core_diag>(
                          vec_, ids_pos[3], ids_pos[2], ids_pos[1], ids_pos[0],
                          m, ctrl_mask)
                    : kernelK<StateVector, Matrix, kernel_core>(
                          vec_, ids_pos[3], ids_pos[2], ids_pos[1], ids_pos[0],
                          m, ctrl_mask);
               break;
          case 5:
#pragma omp parallel
               diag ? kernelK<StateVector, Matrix, kernel_core_diag>(
                          vec_, ids_pos[4], ids_pos[3], ids_pos[2], ids_pos[1],
                          ids_pos[0], m, ctrl_mask)
                    : kernelK<StateVector, Matrix, kernel_core>(
                          vec_, ids_pos[4], ids_pos[3], ids_pos[2], ids_pos[1],
                          ids_pos[0], m, ctrl_mask);
               break;
          default:
               auto message =
                   (boost::format("Run(): cannot apply %u qubits gate") %
                    ids.size())
                       .str();
               LOG(ERROR) << message;
               world_.barrier();
               throw std::runtime_error(message);
     }

#ifndef NDEBUG
     CheckNorm();
#endif

     fused_gates_ = Fusion();

     auto end_run_time = Clock::now();
     auto run_duration = Duration(end_run_time - start_run_time).count();
     VLOG(2) << boost::format("Run(): duration = %.3lf") % run_duration;

     ++stage_runs;
     ++total_runs;
     total_runs_duration += run_duration;

     run_gates = 0;
}

std::tuple<std::map<int, int>, SimulatorMPI::StateVector &>
SimulatorMPI::cheat_local()
{
     std::map<int, int> id2pos;
     for (size_t pos = 0; pos < locals_.size(); ++pos) {
          auto id = locals_[pos];
          id2pos[id] = pos;
     }

     for (size_t pos = 0; pos < globals_.size(); ++pos) {
          auto id = globals_[pos];
          if ((size_t) id != kNotFound_)
               id2pos[id] = pos + locals_.size();
     }

     return make_tuple(id2pos, std::ref(vec_));
}

SimulatorMPI::Float SimulatorMPI::GetProbability(
    const std::vector<bool> &bit_string, const std::vector<Index> &ids)
{
     VLOG(1) << "GetProbability(): ids = " << print(ids);
     VLOG(1) << "GetProbability(): bit_string = " << print(bit_string);
     VLOG(3) << "GetProbability(): locals = " << print(locals_);
     VLOG(3) << "GetProbability(): globals = " << print(globals_);

     uint64_t local_msk = 0;
     uint64_t local_val = 0;
     uint64_t global_msk = 0;
     uint64_t global_val = 0;

     if (ids.size() != bit_string.size()) {
          auto message = "GetProbability(): ids.size() != bit_string.size()";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     for (size_t i = 0; i < ids.size(); ++i) {
          auto id = ids[i];
          bool v = bit_string[i];
          auto pos = ArrayFind(locals_, id);
          if (pos != kNotFound_) {
               local_msk |= 1ul << pos;
               if (v)
                    local_val |= 1ul << pos;
          }
          else {
               pos = ArrayFindSure(globals_, id);
               global_msk |= 1ul << pos;
               if (v)
                    global_val |= 1ul << pos;
          }
     }

     return getProbability_internal(local_msk, local_val, global_msk,
                                    global_val);
}

SimulatorMPI::Complex SimulatorMPI::GetAmplitude(
    const std::vector<bool> &bit_string, const std::vector<Index> &ids) const
{
     VLOG(1) << "GetAmplitude(): bit_string = " << print(bit_string);
     VLOG(1) << "GetAmplitude(): ids = " << print(ids);
     VLOG(3) << "GetAmplitude(): locals = " << print(locals_);
     VLOG(3) << "GetAmplitude(): globals = " << print(globals_);

     auto qureg_size = locals_.size() + globals_.size() -
                       count(globals_.begin(), globals_.end(), kNotFound_);
     if (bit_string.size() != qureg_size || bit_string.size() != ids.size()) {
          auto message = "GetAmplitude(): ids.size() != number of qubits";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     Index rank = 0;
     size_t num = 0;
     size_t check = 0;

     for (size_t i = 0; i < ids.size(); ++i) {
          auto id = ids[i];

          auto pos = ArrayFind(locals_, id);
          if (pos != kNotFound_) {
               num |= bit_string[i] * (1ul << pos);
               check |= (1ul << pos);
          }
          else {
               pos = ArrayFindSure(globals_, id);
               rank |= bit_string[i] * (1ul << pos);
               check |= (1ul << (pos + locals_.size()));
          }
     }

     if ((1ul << qureg_size) - 1 != check) {
          auto message =
              "GetAmplitude(): the second argument must be a permutation of "
              "all allocated qubits.";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     Complex value = vec_[num];
     mpi::broadcast(world_, value, static_cast<int>(rank));
     return value;
}

std::vector<SimulatorMPI::Index> SimulatorMPI::GetQubitsPermutation() const
{
     auto res = locals_;
     res.insert(res.end(), globals_.begin(), globals_.end());

     VLOG(4) << "GetQubitsPermutation(): ids = " << print(res);
     return res;
}

void SimulatorMPI::SetQubitsPermutation(const std::vector<Index> p)
{
     VLOG(1) << "SetQubitsPermutation(): ids = " << print(p);

     locals_ = std::vector<Index>(p.begin(), p.begin() + locals_.size());
     globals_ = std::vector<Index>(p.end() - globals_.size(), p.end());
}

std::vector<SimulatorMPI::Index> SimulatorMPI::GetLocalQubitsPermutation()
{
     VLOG(4) << "GetLocalQubitsPermutation(): ids = " << print(locals_);
     return locals_;
}

std::vector<SimulatorMPI::Index> SimulatorMPI::GetGlobalQubitsPermutation()
{
     VLOG(4) << "GetGlobalQubitsPermutation(): ids = " << print(globals_);
     return globals_;
}

SimulatorMPI::Float SimulatorMPI::Entropy()
{
     Float e = 0;

#pragma omp parallel for schedule(static) reduction(+ : e)
     for (size_t i = 0; i < vec_.size(); ++i) {
          Float pr = abs(vec_[i]) * abs(vec_[i]);
          if (pr > 0)
               e += pr * log2(pr);
     }

     e = mpi::all_reduce(world_, e, std::plus<Float>());
     return -e;
}

std::vector<SimulatorMPI::Index> SimulatorMPI::ExtractLocalCtrls(
    const std::vector<Index> &ctrl) const
{
     std::vector<Index> new_ctrl;
     for (auto i: ctrl) {
          auto pos = ArrayFind(locals_, i);
          if (pos != kNotFound_) {
               new_ctrl.push_back(i);
          }
     }

     return new_ctrl;
}

void SimulatorMPI::ApplyGate(Matrix m, std::vector<Index> ids,
                             std::vector<Index> ctrls)
{
     VLOG(1) << "ApplyGate(): ids = " << print(ids);
     VLOG(1) << "ApplyGate(): ctrls = " << print(ctrls);
     VLOG(3) << "ApplyGate(): globals = " << print(globals_);
     VLOG(3) << "ApplyGate(): locals = " << print(locals_);
     VLOG(4) << "ApplyGate(): matrix = ";
     for (auto &row: m) {
          VLOG(4) << print(row);
     }

     uint64_t flags = get_matrix_props(m, m.size(), m[0].size());
     bool diag = static_cast<bool>(flags & MatProps::IS_DIAG);
     VLOG(3) << "ApplyGate(): is_diagonal = " << diag;

     Index global_id_mask = IdsToBits(ids, globals_);
     auto global_ctrl_mask = IdsToBits(ctrls, globals_);
     auto local_ctrls = ExtractLocalCtrls(ctrls);

     if (ids.size() + local_ctrls.size() > kMaxClusterSize_) {
          VLOG(2) << "ApplyGate(): flushing fusion before huge gate";
          Run();
     }

     if ((rank_ & global_ctrl_mask) != global_ctrl_mask) {
          VLOG(3) << "ApplyGate(): don't apply gate at this rank";
          return;
     }

     if (global_id_mask != 0 && !diag) {
          auto message =
              "ApplyGate(): can't apply non-diagonal gate to global qubits";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     ++run_gates;
     ++stage_gates;
     ++total_gates;

     if (ids.size() + local_ctrls.size() > kMaxClusterSize_) {
          VLOG(2) << "ApplyGate(): added huge gate to fused_gates directly";
          fused_gates_.insert(m, flags, ids, ctrls);
          return;
     }

     auto ok_bit = [&](const Index msk) {
          for (size_t i = 0; (1ul << i) < m.size(); ++i) {
               size_t pos = ArrayFind(globals_, ids[i]);
               if (pos != kNotFound_) {
                    if ((msk >> i & 1) != (rank_ >> pos & 1)) {
                         return false;
                    }
               }
          }
          return true;
     };

     size_t new_size = 0;
     for (size_t i = 0; i < m.size(); ++i) {
          new_size += ok_bit(i);
     }

     Matrix new_matrix(new_size);
     for (auto &i: new_matrix) {
          i.resize(new_size);
     }

     size_t i_pos = 0;
     for (size_t i = 0; i < m.size(); ++i) {
          if (!ok_bit(i)) {
               continue;
          }

          size_t j_pos = 0;
          for (size_t j = 0; j < m.size(); ++j) {
               if (!ok_bit(j)) {
                    continue;
               }

               new_matrix[i_pos][j_pos] = m[i][j];
               ++j_pos;
          }
          ++i_pos;
     }

     ctrls = local_ctrls;
     m = std::move(new_matrix);
     Fusion::add_controls(m, ids, ctrls);
     ctrls.clear();
     ids.erase(std::remove_if(ids.begin(), ids.end(),
                              [&](const Index id) {
                                   return ArrayFind(globals_, id) != kNotFound_;
                              }),
               ids.end());

     VLOG(3) << "ApplyGate(): (processed) ids = " << print(ids);
     VLOG(4) << "ApplyGate(): (processed) matrix = ";
     for (auto &row: m) {
          VLOG(4) << print(row);
     }

     fused_gates_.insert(m, flags, ids, ctrls);
}

void SimulatorMPI::calcLocalApproxDistribution(const size_t n)
{
     block_distribution.resize(n, bc::default_init);
     FillVector<bc::vector<Float>>(block_distribution.begin(),
                                   block_distribution.end(), 0.0);

     CHECK(vec_.size() % n == 0);
     size_t K = vec_.size() / n;

#ifndef _MSC_VER
#     pragma omp distribute parallel for
#else
#     pragma omp parallel for
#endif  // _MSC_VER
     for (size_t i = 0; i < n; ++i) {
          for (size_t j = 0; j < K; ++j) {
               block_distribution[i] += std::norm(vec_[i * K + j]);
          }
     }

     std::partial_sum(block_distribution.begin(), block_distribution.end(),
                      block_distribution.begin());
}

SimulatorMPI::Float SimulatorMPI::getProbability_internal(uint64_t local_msk,
                                                          uint64_t local_val,
                                                          uint64_t global_msk,
                                                          uint64_t global_val)
{
     VLOG(4) << boost::format(
                    "getProbability_internal(): local_msk: %d, local_val: %d, "
                    "global_msk: %d, global_val: %d") %
                    local_msk % local_val % global_msk % global_val;
     VLOG(4) << boost::format("getProbability_internal(): local state vector: ")
             << print(vec_);
     Float local_probability = 0.;
     if ((rank_ & global_msk) == global_val) {
#pragma omp parallel for reduction(+ : local_probability) schedule(static)
          for (size_t i = 0; i < vec_.size(); ++i) {
               if ((i & local_msk) == local_val) {
                    local_probability += std::norm(vec_[i]);
               }
          }
     }

     Float probability =
         mpi::all_reduce(world_, local_probability, std::plus<Float>());
     VLOG(1) << boost::format(
                    "getProbability_internal(): local_probability = %.3lf; "
                    "probability = %.3lf") %
                    local_probability % probability;

     return probability;
}

void SimulatorMPI::normalize(Float norm, uint64_t local_msk, uint64_t local_val,
                             uint64_t global_msk, uint64_t global_val)
{
     norm = 1. / std::sqrt(norm);

     if ((rank_ & global_msk) != global_val) {
          FillVector<StateVector>(vec_.begin(), vec_.end(), 0);
     }
     else {
#pragma omp parallel for schedule(static)
          for (size_t i = 0; i < vec_.size(); ++i) {
               if ((i & local_msk) != local_val) {
                    vec_[i] = 0.;
               }
               else {
                    vec_[i] *= norm;
               }
          }
     }

#ifndef NDEBUG
     CheckNorm();
#endif
}

std::vector<bool> SimulatorMPI::MeasureQubits(std::vector<Index> const &ids)
{
     VLOG(1) << "MeasureQubits(): ids = " << print(ids);

     auto start_measure_time = Clock::now();

     uint64_t n = std::min(vec_.size(), std::size_t(1ul << 15));
     calcLocalApproxDistribution(n);

     total_block_distribution.resize(n * world_.size(), bc::default_init);

     mpi::all_gather(world_, block_distribution.data(), static_cast<int>(n),
                     total_block_distribution.data());

     Float shift = 0;
     for (int i = 0; i < world_.size(); ++i) {
          Float new_shift = total_block_distribution[i * n + n - 1];
          for (size_t j = 0, k = i * n; j < n; ++j, ++k) {
               total_block_distribution[k] += shift;
          }
          shift += new_shift;
     }

     Float rnd = rng_();
     uint64_t i = 0;
     for (; i < total_block_distribution.size(); ++i) {
          if (rnd <= total_block_distribution[i])
               break;
     }

     uint64_t src_rank = i / n;
     uint64_t src_index = i % n;

     VLOG(1)
         << boost::format(
                "MeasureQubits(): rnd = %.3lf; src_rank: %u, src_index: %u") %
                rnd % src_rank % src_index;

     auto res = std::vector<bool>(ids.size());
     if (static_cast<int>(src_rank) == world_.size()) {
          auto message =
              (boost::format("MeasureQubits(): Random number %.3lf > norm "
                             "partial sum %.3lf") %
               rnd % total_block_distribution.back())
                  .str();
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     uint64_t block_size = vec_.size() / n;
     uint64_t k = 0;
     if (rank_ == static_cast<int>(src_rank)) {
          shift = 0;
          if (i > 0) {
               shift = total_block_distribution[i - 1];
          }

          k = src_index * block_size;
          for (uint64_t j = 0; j < block_size; ++j, ++k) {
               shift += std::norm(vec_[k]);
               if (shift >= rnd) {
                    break;
               }
          }
     }

     uint64_t res_index = (src_rank << locals_.size()) + k;
     mpi::broadcast(world_, res_index, static_cast<int>(src_rank));

     VLOG(1) << boost::format("MeasureQubits(): res_index: %u") % res_index;

     // determine result vector (boolean values for each qubit)
     // and create local_msk to detect bad entries (i.e., entries that don't
     // agree with measurement)
     uint64_t local_msk = 0;
     uint64_t local_val = 0;
     uint64_t global_msk = 0;
     uint64_t global_val = 0;

     for (i = 0; i < ids.size(); ++i) {
          auto id = ids[i];
          auto pos = ArrayFind(locals_, id);
          if (pos != kNotFound_) {
               local_msk |= 1ul << pos;
               if (res_index & (1ul << pos)) {
                    local_val |= 1ul << pos;
                    res[i] = true;
               }
          }
          else {
               pos = ArrayFindSure(globals_, id);
               global_msk |= 1ul << pos;
               if (src_rank & (1ul << pos)) {
                    global_val |= 1ul << pos;
                    res[i] = true;
               }
          }
     }

     auto norm =
         getProbability_internal(local_msk, local_val, global_msk, global_val);

     normalize(norm, local_msk, local_val, global_msk, global_val);

     auto measure_duration =
         Duration(Clock::now() - start_measure_time).count();
     total_measure_duration += measure_duration;
     VLOG(1) << boost::format("MeasureQubits(): duration = %.3lf") %
                    measure_duration;

     return res;
}

void SimulatorMPI::collapseWaveFunction(const std::vector<Index> &ids,
                                        const std::vector<bool> &values)
{
     VLOG(1) << boost::format("collapseWaveFunction(): ids: ") << print(ids);
     VLOG(1) << boost::format("collapseWaveFunction(): values: ")
             << print(values);

     uint64_t local_msk = 0;
     uint64_t local_val = 0;
     uint64_t global_msk = 0;
     uint64_t global_val = 0;

     if (ids.size() != values.size()) {
          auto message = "collapseWaveFunction(): ids.size() != values.size()";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     for (size_t i = 0; i < ids.size(); ++i) {
          auto id = ids[i];
          bool v = values[i];
          auto pos = ArrayFind(locals_, id);
          if (pos != kNotFound_) {
               local_msk |= 1ul << pos;
               if (v)
                    local_val |= 1ul << pos;
          }
          else {
               pos = ArrayFindSure(globals_, id);
               global_msk |= 1ul << pos;
               if (v)
                    global_val |= 1ul << pos;
          }
     }

     auto norm =
         getProbability_internal(local_msk, local_val, global_msk, global_val);

     if (norm < 1.e-12) {
          auto message =
              "collapseWaveFunction(): Invalid collapse! Probability is ~0.";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     normalize(norm, local_msk, local_val, global_msk, global_val);
}

void SimulatorMPI::SwapQubitsWrapper(const std::vector<Index> &swap_pairs)
{
     EndStage();
     auto start_swap_time = Clock::now();

     VLOG(1) << "SwapQubitsWrapper(): swap_pairs = " << printPairs(swap_pairs);

     SwapQubits(swap_pairs);

     auto swap_duration = Duration(Clock::now() - start_swap_time).count();
     total_swap_duration += swap_duration;

     int qubits = static_cast<int>(swap_pairs.size() / 2);
     Float frac = 1 - Float{1} / (1ul << qubits);
     Float swapped_bytes = 16 * frac * (1ul << locals_.size());
     auto bandwidth =
         (Float{1} / (1ul << 30)) * swapped_bytes * 8 / swap_duration;
     VLOG(1) << boost::format(
                    "SwapQubitsWrapper(): duration = %.3lf; qubits = %d; "
                    "bandwidth = %.3lf Gb/s") %
                    swap_duration % (swap_pairs.size() / 2) % bandwidth;

     StartStage();
}

void SimulatorMPI::SwapQubits(const std::vector<Index> &swap_pairs)
{
     VLOG(1) << "SwapQubits(): swap_pairs = " << printPairs(swap_pairs);
     VLOG(3) << "SwapQubits(): locals = " << print(locals_);
     VLOG(3) << "SwapQubits(): globals = " << print(globals_);

     auto color = static_cast<uint64_t>(rank_);
     for (size_t i = 0; i < swap_pairs.size(); i += 2) {
          color &= ~(1ul << ArrayFindSure(globals_, swap_pairs[i]));
     }

     static std::vector<uint64_t> swapBits;
     std::map<Index, Index> pos;
     for (size_t i = 0; i < swap_pairs.size(); i += 2) {
          pos[swap_pairs[i]] = ArrayFindSure(globals_, swap_pairs[i]);
          pos[swap_pairs[i + 1]] = ArrayFindSure(locals_, swap_pairs[i + 1]);
     }

     if (pos.size() != swap_pairs.size()) {
          auto message = "SwapQubits(): each qubit should be unique";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     q2bits_ng<uint64_t>(swapBits, swap_pairs, pos);
     auto comm = world_.split(static_cast<int>(color));
     VLOG(3) << boost::format("SwapQubits(): color = %d; comm.size() = %d") %
                    color % comm.size();

     // this check is made to satisfy automated code inspection
     uint64_t comm_size = comm.size();
     if (comm_size == 0) {
          auto message = "SwapQubits(): world.split() returned comm of 0 size";
          LOG(ERROR) << message;
          world_.barrier();
          throw std::runtime_error(message);
     }

     buffs_.resize(1ul << 18);

     SwapperMT s(world_, vec_, static_cast<uint64_t>(rank_), locals_.size(),
                 comm_size, buffs_, swap_pairs.size() / 2);
     s.doSwap(rank_, comm, color, swapBits);

     for (size_t i = 0; i < swap_pairs.size(); i += 2) {
          auto pos_global = ArrayFindSure(globals_, swap_pairs[i]);
          auto pos_local = ArrayFindSure(locals_, swap_pairs[i + 1]);
          std::swap(locals_[pos_local], globals_[pos_global]);
     }

     VLOG(3) << "SwapQubits(): (processed) locals = " << print(locals_);
     VLOG(3) << "SwapQubits(): (processed) globals = " << print(globals_);
}

void SimulatorMPI::StartStage()
{
     start_stage_time = Clock::now();
     ++total_stages;
     stage_runs = 0;
     stage_gates = 0;
}

void SimulatorMPI::EndStage()
{
     auto stage_duration = Duration(Clock::now() - start_stage_time).count();
     VLOG(1) << boost::format(
                    "EndStage(): duration = %.3lf; gates = %d; runs = %d; "
                    "gates/run = %.3lf; time/run = %.3lf") %
                    stage_duration % stage_gates % stage_runs %
                    (stage_gates / static_cast<Float>(stage_runs)) %
                    (stage_duration / stage_runs);
}
