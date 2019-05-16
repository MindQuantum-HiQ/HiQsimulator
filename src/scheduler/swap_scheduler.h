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

using std::map;
using std::vector;

class SwapScheduler
{
public:
     // Chooses which qubits have to be local during next stage.
     // Uses backtracking with limited number of branches _num_splits.
     // In worst case works in O(_num_splits * num_gates) time, O(num_gates)
     // memory
     vector<id_num_t> ScheduleSwap();

     SwapScheduler(vector<vector<id_num_t>> _gate,
                   vector<vector<id_num_t>> _gate_ctrl, vector<bool> _gate_diag,
                   int _num_splits, int _num_locals, bool fuse);

     ~SwapScheduler()
     {}

private:
     const int NumSplits;
     const int NumLocals;
     vector<msk_t> gate, gate_ctrl;
     vector<bool> gate_diag;
     vector<int> gate_weight;
     vector<id_num_t> pos_to_id;
     map<id_num_t, int> id_to_pos;
     int best_ans;
     msk_t best_locals;

     // Returns pos-th gate type.
     // 0 - not diagonal
     // 1 - diagonal
     inline int GateType(const int pos) const
     {
          return gate_diag[pos];
     }

     // Determines is it possible to apply pos-th gate in given situation.
     inline bool CanTake(const int pos, const msk_t cur_locals,
                         const msk_t cur_bad) const
     {
          if (inter(unite(gate_ctrl[pos], gate[pos]), cur_bad)) {
               return false;
          }

          if (GateType(pos) == 0) {
               return count_bits(unite(gate[pos], cur_locals)) <= NumLocals;
          }
          else {
               return true;
          }
     }

     // Fuses i-th single-qubit gate to gate j-th (IMPORTANT: i-th gate has to
     // be removed)
     inline void FuseSingleQubitGateToThis(int i, int j)
     {
          gate_weight[j] += gate_weight[i];
          gate_weight[i] = 0;

          if (!gate_diag[i]) {
               gate_diag[j] = false;
          }

          if (inter(gate_ctrl[j], gate[i])) {
               gate_ctrl[j] ^=
                   gate[i];  // Remove bit gate[i] from gate_ctrl[j] mask
               gate[j] = unite(gate[j], gate[i]);
          }
     }

     // Fuses i-th single-qubit gate with prev gate containing this qubit
     // (IMPORTANT: in case of success, i-th gate has to be removed)
     inline bool FuseSingleQubitToPrevInterGate(int i)
     {
          for (int j = i - 1; j >= 0; --j) {
               if (inter(gate[i], unite(gate[j], gate_ctrl[j]))) {
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
          for (int j = i + 1; j < static_cast<int>(gate.size()); ++j) {
               if (inter(gate[i], unite(gate[j], gate_ctrl[j]))) {
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
