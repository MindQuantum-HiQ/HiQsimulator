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

#ifndef SIMULATORMPI_HPP
#define SIMULATORMPI_HPP

#include <chrono>
#include <complex>
#include <functional>
#include <random>
#include <vector>

#include <boost/container/vector.hpp>
#include <boost/mpi.hpp>
#include "simulator-mpi/fusion_mpi.hpp"
#include "simulator-mpi/alignedallocator.hpp"
#include "simulator-mpi/SwapArrays.hpp"


#if defined(NOINTRIN) || (!defined(INTRIN) && !defined(INTRIN_CF))
#include "simulator-mpi/kernels/nointrin/kernels.hpp"
#include "simulator-mpi/kernels/nointrin/kernels_diag.hpp"
using nointrin::kernelK;
using nointrin::kernelK_diag1;
using nointrin::kernel_core;
using nointrin::kernel_core_diag;
#elif defined(INTRIN_CF)
#include "simulator-mpi/kernels/intrin_cf/kernels.hpp"
#include "simulator-mpi/kernels/intrin/kernels_diag.hpp"
using intrin_cf::kernelK;
using intrin::kernelK_diag1;
using intrin_cf::kernel_core;
using intrin::kernel_core_diag;
#else
#include "simulator-mpi/kernels/intrin/kernels.hpp"
#include "simulator-mpi/kernels/intrin/kernels_diag.hpp"
using intrin::kernelK;
using intrin::kernelK_diag1;
using intrin::kernel_core;
using intrin::kernel_core_diag;
#endif

namespace mpi = boost::mpi;
namespace bc = boost::container;

#ifdef _WIN32
#  ifdef SIMULATOR_LIBRARY_EXPORT
#    define EXPORT_API __declspec(dllexport)
#  else
#    define EXPORT_API __declspec(dllimport)
#  endif // SIMULATOR_LIBRARY_EXPORT
#else
#  define EXPORT_API
#endif // _WIN32

class EXPORT_API SimulatorMPI {
 public:
  using Index = int64_t;
  using Float = double;
  using Complex = std::complex<Float>;
  using Matrix = std::vector<std::vector<Complex, aligned_allocator<Complex, 64>>>;
  using StateVector = bc::vector<Complex, aligned_allocator<Complex, 64>>;
  using RndEngine = std::mt19937;
  using Duration = std::chrono::duration<Float>;
  using Clock = std::chrono::high_resolution_clock;

  static constexpr size_t kNotFound_ = static_cast<size_t>(-1);

  SimulatorMPI(mpi::communicator aWorld, uint64_t seed, size_t max_local, size_t max_cluster_size);
  SimulatorMPI(uint64_t seed, size_t max_local, size_t max_cluster_size);
  SimulatorMPI(const SimulatorMPI &other) = delete;
  ~SimulatorMPI();

  void AllocateQubit(Index id);
  void AllocateQureg(const std::vector<Index> &ids, Complex init = 0);
  void DeallocateQubit(Index id);
  void Run();
  void ApplyGate(Matrix m, std::vector<Index> ids, std::vector<Index> ctrl);
  std::vector<bool> MeasureQubits(std::vector<Index> const &ids);
  void SwapQubitsWrapper(const std::vector<Index> &swap_pairs_ids);
  Complex GetAmplitude(const std::vector<bool> &bit_string, const std::vector<Index> &ids) const;
  Float GetProbability(const std::vector<bool> &bit_string, const std::vector<Index> &ids);
  Float Entropy();
  std::vector<Index> GetQubitsPermutation() const;
  std::vector<Index> GetLocalQubitsPermutation();
  std::vector<Index> GetGlobalQubitsPermutation();
  void SetQubitsPermutation(std::vector<Index>);
  std::tuple<std::map<int, int>, StateVector&> cheat_local();
  void collapseWaveFunction(const std::vector<Index>& ids, const std::vector<bool>& values);

  template<class F, class QuReg>
  void emulate_math(F const &, QuReg, const std::vector<Index> &, unsigned = 1) {
    world_.barrier();
    throw std::runtime_error("SimulatorMPI::emulate_math() is not supported");
  }

  size_t LocalQubitsCount() const { return locals_.size(); }
  size_t GlobalQubitsCount() const { return globals_.size() - std::count(globals_.begin(), globals_.end(), -1); }
  size_t TotalQubitsCount() const { return LocalQubitsCount() + GlobalQubitsCount(); }

  mpi::environment env_;
  mpi::communicator world_;
  StateVector vec_;

 private:
  const Float kMaxFloatError_;
  const size_t kMinLocal_;
  const size_t kMaxLocal_;
  const size_t kMaxGlobal_;
  size_t kMaxClusterSize_;
  std::vector<Index> locals_;
  std::vector<Index> globals_;

  Fusion fused_gates_;
  RndEngine rnd_eng_;
  std::function<double()> rng_;

  int rank_;
  SwapBuffers<Complex> buffs_;

  int run_gates = 0;
  int stage_gates = 0;
  int total_gates = 0;
  int stage_runs = 0;
  int total_runs = 0;
  int total_stages = 0;
  Float total_runs_duration = 0.0;
  Float total_swap_duration = 0.0;
  Float total_measure_duration = 0.0;
  Float total_alloc_duration = 0.0;
  Float total_dealloc_duration = 0.0;
  Clock::time_point start_stage_time;
  Clock::time_point start_time;

  void AllocateLocalQubit(Index id);
  void AllocateGlobalQubit(Index id);
  void DeallocateLocalQubit(Index id);
  void DeallocateGlobalQubit(Index id);
  void SwapQubits(const std::vector<Index> &swap_pairs_ids);
  void CheckNorm();
  size_t ArrayFind(const std::vector<Index> &v, Index val) const;
  size_t ArrayFindSure(const std::vector<Index> &v, Index val) const;
  uint64_t IdsToBits(const std::vector<Index> &ids, const std::vector<Index> &perm) const;
  std::vector<Index> ExtractLocalCtrls(const std::vector<Index> &ctrl) const;
  void StartStage();
  void EndStage();

  bc::vector<Float> block_distribution;
  bc::vector<Float> total_block_distribution;

  void calcLocalApproxDistribution(size_t n);

  Float getProbability_internal(uint64_t local_msk, uint64_t local_val, uint64_t global_msk, uint64_t global_val);
  void normalize(Float norm, uint64_t local_msk, uint64_t local_val, uint64_t global_msk, uint64_t global_val);
};

#endif // SIMULATORMPI_HPP
