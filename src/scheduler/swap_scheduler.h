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

#ifndef SCHEDULER_SWAP_SCHEDULER_H
#define SCHEDULER_SWAP_SCHEDULER_H

#include <map>
#include <vector>

#include "definitions.h"

class SwapScheduler
{
public:
     // Chooses which qubits have to be local during next stage.
     // Uses backtracking with limited number of branches _num_splits.
     // In worst case works in O(_num_splits * num_gates) time, O(num_gates)
     // memory
     std::vector<id_num_t> ScheduleSwap();

     SwapScheduler(const std::vector<std::vector<id_num_t>>& gate,
                   const std::vector<std::vector<id_num_t>>& gate_ctrl,
                   const std::vector<bool>& gate_diag, int num_splits,
                   int num_locals, bool fuse);

     ~SwapScheduler()
     {}

private:
     const int num_splits_;
     const int num_locals_;
     std::vector<msk_t> gate_, gate_ctrl_;
     std::vector<bool> gate_diag_;
     std::vector<int> gate_weight_;
     std::vector<id_num_t> pos_to_id_;
     std::map<id_num_t, int> id_to_pos_;
     int best_ans_;
     msk_t best_locals_;

     // Returns pos-th gate type.
     // 0 - not diagonal
     // 1 - diagonal
     inline int GateType(const int pos) const
     {
          return gate_diag_[pos];
     }

     // Determines is it possible to apply pos-th gate in given situation.
     inline bool CanTake(const int pos, const msk_t cur_locals,
                         const msk_t cur_bad) const
     {
          if (inter(unite(gate_ctrl_[pos], gate_[pos]), cur_bad)) {
               return false;
          }

          if (GateType(pos) == 0) {
               return count_bits(unite(gate_[pos], cur_locals)) <= num_locals_;
          }
          else {
               return true;
          }
     }

     // Fuses i-th single-qubit gate to gate j-th (IMPORTANT: i-th gate has to
     // be removed)
     inline void FuseSingleQubitGateToThis(int i, int j)
     {
          gate_weight_[j] += gate_weight_[i];
          gate_weight_[i] = 0;

          if (!gate_diag_[i]) {
               gate_diag_[j] = false;
          }

          if (inter(gate_ctrl_[j], gate_[i])) {
               gate_ctrl_[j] ^=
                   gate_[i];  // Remove bit gate[i] from gate_ctrl[j] mask
               gate_[j] = unite(gate_[j], gate_[i]);
          }
     }

     // Fuses i-th single-qubit gate with prev gate containing this qubit
     // (IMPORTANT: in case of success, i-th gate has to be removed)
     inline bool FuseSingleQubitToPrevInterGate(int i)
     {
          for (int j = i - 1; j >= 0; --j) {
               if (inter(gate_[i], unite(gate_[j], gate_ctrl_[j]))) {
                    FuseSingleQubitGateToThis(i, j);
                    return true;
               }
          }
          return false;
     }

     // Fuses i-th gate with next gate containing this qubit (IMPORTANT: in case
     // of success, i-th gate has to be removed)
     inline bool FuseSingleQubitToNextInterGate(int i)
     {
          for (int j = i + 1; j < static_cast<int>(gate_.size()); ++j) {
               if (inter(gate_[i], unite(gate_[j], gate_ctrl_[j]))) {
                    FuseSingleQubitGateToThis(i, j);
                    return true;
               }
          }
          return false;
     }

     // Backtracking method to choose local and global qubits.
     int Rec(int pos, msk_t cur_locals, msk_t cur_bad, int cur_ans,
             int splits_left);

     // Tries to fuse single qubit gates with next/prev gate containing this
     // qubit.
     void FuseSingleQubitGates();
};

#endif  // SCHEDULER_SWAP_SCHEDULER_H
