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

#include <boost/container/vector.hpp>
#include <boost/mpi.hpp>
#include <chrono>
#include <complex>
#include <functional>
#include <random>
#include <vector>

#include "simulator-mpi/SwapArrays.hpp"
#include "simulator-mpi/alignedallocator.hpp"
#include "simulator-mpi/fusion_mpi.hpp"

namespace mpi = boost::mpi;
namespace bc = boost::container;

#ifdef _WIN32
#     ifdef SIMULATOR_LIBRARY_EXPORT
#          define EXPORT_API __declspec(dllexport)
#     else
#          define EXPORT_API __declspec(dllimport)
#     endif  // SIMULATOR_LIBRARY_EXPORT
#else
#     define EXPORT_API
#endif  // _WIN32


/*!
 * \brief SimulatorMPI is a distributed state vector quantum circuit simulator. It uses qubit reordering technique to manage distributed quantum state.

 References: *Massively parallel quantum computer simulator, K. De Raedt et al. Computer Physics Communications 176 (2007) 121–136*
 */
class EXPORT_API SimulatorMPI
{
public:
     using Index = int64_t;
     using Float = double;
     using Complex = std::complex<Float>;
     using Matrix
         = std::vector<std::vector<Complex, aligned_allocator<Complex, 64>>>;
     using StateVector = bc::vector<Complex, aligned_allocator<Complex, 64>>;
     using RndEngine = std::mt19937;
     using Duration = std::chrono::duration<Float>;
     using Clock = std::chrono::high_resolution_clock;

     static constexpr size_t kNotFound_ = static_cast<size_t>(-1);
     //! Constructor
     /*!
      * \param aWorld A MPI communicator
      * \param seed Seed for pseudo-random number generator
      * \param max_local Maximum number of local qubits
      * \param max_cluster_size Maximum number of qubits in fused multi-qubit gate
      */
     SimulatorMPI(uint64_t seed, size_t max_local, size_t max_cluster_size);

     //! Constructor
     /*!
      * \brief With this constructor you can define your own MPI communicator which
               must be able to work with `2^N` processes, where N is the number of qubits.
      * \param aWorld A MPI communicator
      * \param seed Seed for pseudo-random number generator
      * \param max_local Maximum number of local qubits
      * \param max_cluster_size Maximum number of qubits in fused multi-qubit gate
      */
     SimulatorMPI(mpi::communicator aWorld, uint64_t seed, size_t max_local,
                  size_t max_cluster_size);

     //! Copy constructor
     /*!
      * \note Actual declaration is `SimulatorMPI(const SimulatorMPI &other) = delete`.\n
      * Copying of the simulator is prohibited.
      */
     SimulatorMPI(const SimulatorMPI &other) = delete;

     //! Destructor
     ~SimulatorMPI();

     //! Allocate one qubit with a given ID
     /*!
      * \param id ID of the qubit to allocate
      */
     void AllocateQubit(Index id);

     //! Allocate the quantum register qureg with given qubits IDs
     /*!
      * \param ids Array of qubit IDs
      * \param init Value to be assigned to all amplitudes
      * \throw std::runtime_error if this is not the first allocation and init is not equal 0.0
      */
     void AllocateQureg(const std::vector<Index> &ids, Complex init = 0);

     //! De-allocate one qubit with a given ID
     /*!
      * \param id ID of the qubit to de-allocate
      */
     void DeallocateQubit(Index id);

     /*!
      * \brief Actually make calculations:
               + fuse applied (used ApplyGate()) gates into a single multi-qubit gate
               + calculate the product of the resulting gate by state vector
      * \throw std::runtime_error if gate cannot be applied
      */
     void Run();

     //! Save gate and than apply it to function Run()
     /*!
      * \param m Matrix of the gate
      * \param ids Array of qubit IDs
      * \param ctrl Array of control qubits
      * \throw std::runtime_error if gate is non-diagonal
      */
     void ApplyGate(Matrix m, std::vector<Index> ids, std::vector<Index> ctrl);

     //! Measure qubits state
     /*!
      * \param ids Array of qubit IDs to measure
      * \return Bit array of measurement results (containing either True or False)
      */
     std::vector<bool> MeasureQubits(std::vector<Index> const &ids);

     //! See SwapQubits()
     /*!
      * \brief Print logs: duration, qubits, bandwidth
      */
     void SwapQubitsWrapper(const std::vector<Index> &swap_pairs_ids);

     //! Swap global and local qubits in pairs
     /*!
      * \param swap_pairs_ids Array of qubit pairs IDs (there is a global qubit
               ID on the first place, a local - on the second)
      * \throw std::runtime_error if qubits are not unique
      */
     void SwapQubits(const std::vector<Index> &swap_pairs_ids);

     /*!
      * \brief Return the amplitude of the supplied amplitude index
      * \param bit_string Index amplitude
      * \param ids Array of qubit IDs
      * \return Amplitude of the provided bit_string
      * \throw std::runtime_error if the number of IDs does not match the number
               of qubits or if any of the desired qubits are not already allocated
      */
     Complex GetAmplitude(const std::vector<bool> &bit_string,
                          const std::vector<Index> &ids) const;

     /*!
      * \brief Return the probability of the outcome bit_string when measuring
      *        the quantum register qureg.
      * \param bit_string Measurement outcome
      * \param ids Array of qubit IDs
      * \return Probability of measuring the provided bit string
      * \throw std::runtime_error if the number of IDs does not match the length of bit_string
      * \note Doesn't actually make the calculations
      */
     Float GetProbability(const std::vector<bool> &bit_string,
                          const std::vector<Index> &ids);

     /*!
      * \brief Return the entropy of quantum state.
      */
     Float Entropy();

     /*!
      * \brief Get permutation of qubits.
      * \return Array of qubit IDs
      */
     std::vector<Index> GetQubitsPermutation() const;

     /*!
      * \brief Get permutation of local qubits.
      * \return Array of qubit IDs
      */
     std::vector<Index> GetLocalQubitsPermutation();

     /*!
      * \brief Get permutation of global qubits.
      * \return Array of qubit IDs
      */
     std::vector<Index> GetGlobalQubitsPermutation();

     //! Set permutation to all qubits without actually reordering quantum state vector
     /*!
      * \param p Array of qubit IDs
      */
     void SetQubitsPermutation(std::vector<Index> p);

     /*!
      * \brief Access the ordering of the qubits and this MPI process's part of
        state vector directly.
      * \return A tuple where the first entry is a dictionary mapping qubit
                indices to bit-locations (all qubits) and the second entry is the local part
                of state vector.
      */
     std::tuple<std::map<int, int>, StateVector &> cheat_local();

     //! Collapse a quantum register into a classical basis state
     /*!
      * \param ids Array of qubit IDs
      * \param values Measurement outcome for each of the qubits in `qureg`.
      * \throw std::runtime_error if the number of IDs does not match the number of values
               or measured probability less than 1.e-12
      */
     void collapseWaveFunction(const std::vector<Index> &ids,
                               const std::vector<bool> &values);


     //! This function is not supported
     template <class F, class QuReg>
     void emulate_math(F const &, QuReg, const std::vector<Index> &,
                       unsigned = 1)
     {
          world_.barrier();
          throw std::runtime_error(
              "SimulatorMPI::emulate_math() is not supported");
     }

     /*!
      * \return Number of local qubits
      */
     size_t LocalQubitsCount() const
     {
          return locals_.size();
     }

     /*!
      * \return Number of global qubits
      */
     size_t GlobalQubitsCount() const
     {
          return globals_.size()
                 - std::count(globals_.begin(), globals_.end(), -1);
     }

     /*!
      * \return Number of all qubits
      */
     size_t TotalQubitsCount() const
     {
          return LocalQubitsCount() + GlobalQubitsCount();
     }

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
     void CheckNorm();
     size_t ArrayFind(const std::vector<Index> &v, Index val) const;
     size_t ArrayFindSure(const std::vector<Index> &v, Index val) const;
     uint64_t IdsToBits(const std::vector<Index> &ids,
                        const std::vector<Index> &perm) const;
     std::vector<Index> ExtractLocalCtrls(const std::vector<Index> &ctrl) const;
     void StartStage();
     void EndStage();

     bc::vector<Float> block_distribution;
     bc::vector<Float> total_block_distribution;

     void calcLocalApproxDistribution(size_t n);

     Float getProbability_internal(uint64_t local_msk, uint64_t local_val,
                                   uint64_t global_msk, uint64_t global_val);
     void normalize(Float norm, uint64_t local_msk, uint64_t local_val,
                    uint64_t global_msk, uint64_t global_val);
};

#endif  // SIMULATORMPI_HPP
