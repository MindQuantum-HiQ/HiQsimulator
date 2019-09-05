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

#ifndef SCHEDULER_CLUSTER_SCHEDULER
#define SCHEDULER_CLUSTER_SCHEDULER

#include <glog/logging.h>

#include <iostream>
#include <map>
#include <vector>

#include "definitions.h"

class ClusterScheduler
{
public:
     // Finds the largest subset of gates than can be fused into one cluster
     // Uses backtracking on qubits ids
     // Works in O(C(num_qubits, _cluster_size) * num_gates) time,
     // O(C(num_qubits, _cluster_size)) memory

     /*!
      * \brief Find the largest subset of gates than can be fused into one cluster. Use backtracking on qubits IDs. Print logs.
      * \return Indices of gates (from list gate) from one cluster
      */
     std::vector<int> ScheduleCluster();

     //! Constructor
     /*!
      * \param gate For each gate (from the analyzed quantum circuit) a list of qubits on which it acts
      * \param gate_ctrl For each gate a list of control qubits on which it acts
      * \param gate_diag For each gate **false** (non-diagonal gate) or **true** (diagonal gate)
      * \param locals IDs of local qubits
      * \param globals IDs of global qubits
      * \param cluster_size Maximum number of qubits in fused multi-qubit gate
      */
     ClusterScheduler(const std::vector<std::vector<id_num_t>>& gate,
                      const std::vector<std::vector<id_num_t>>& gate_ctrl,
                      std::vector<bool> gate_diag,
                      const std::vector<id_num_t>& locals,
                      const std::vector<id_num_t>& globals, int cluster_size);

     //! Destructor
     ~ClusterScheduler()
     {}

private:
     const int cluster_size_;
     int best_ans_;
     msk_t best_cluster_;
     msk_t locals_;
     msk_t globals_;
     std::map<msk_t, msk_t> used_;
     std::vector<msk_t> gate_, gate_ctrl_;
     std::vector<bool> gate_diag_;
     std::vector<id_num_t> pos_to_id_;
     std::map<id_num_t, int> id_to_pos_;

     // Generates qubits combinations, containing no more than ClusterSize
     // qubits
     void Rec(msk_t cur_cluster, msk_t bit);

     // Tries to improve answer by given qubits combination
     inline void CalcCombination(msk_t cur_cluster)
     {
          if (used_[cur_cluster] != 0) {
               return;
          }
          msk_t cur_bad = 0;
          int cur_ans = 0;

          for (int i = 0; i < static_cast<int>(gate_.size()); ++i) {
               if (CanTake(i, cur_cluster, cur_bad)) {
                    ++cur_ans;
               }
               else {
                    cur_bad = unite(cur_bad, unite(gate_[i], gate_ctrl_[i]));
               }
          }

          if (cur_ans > best_ans_
              || (cur_ans == best_ans_
                  && count_bits(cur_cluster) < count_bits(best_cluster_))) {
               best_ans_ = cur_ans;
               best_cluster_ = cur_cluster;
          }
     }

     // Returns gates to be used in current cluster
     inline std::vector<int> GetComamndsFromMsk() const
     {
          msk_t cur_bad = 0;

          std::vector<int> ans;
          for (int i = 0; i < static_cast<int>(gate_.size()); ++i) {
               if (CanTake(i, best_cluster_, cur_bad)) {
                    ans.push_back(i);
               }
               else {
                    cur_bad = unite(cur_bad, unite(gate_[i], gate_ctrl_[i]));
               }
          }

          CHECK(static_cast<int>(ans.size()) == best_ans_)
              << "GetComamndsFromMsk(): Internal error: sizes mismatch";
          return ans;
     }

     // Returns huge gate to be used in current cluster if possible
     inline std::vector<int> GetCommandsFromLocals() const
     {
          msk_t cur_bad = 0;

          for (int i = 0; i < static_cast<int>(gate_.size()); ++i) {
               if (CanTake(i, locals_, cur_bad)) {
                    return {i};
               }
               else {
                    cur_bad = unite(cur_bad, unite(gate_[i], gate_ctrl_[i]));
               }
          }

          return {};
     }

     // Returns pos-th gate type.
     // 0 - not diagonal
     // 1 - diagonal
     inline int GateType(const int pos) const
     {
          return gate_diag_[pos];
     }

     // Determines if is it possible to apply pos-th gate in given situation.
     inline bool CanTake(const int pos, const msk_t cur_cluster,
                         const msk_t cur_bad) const
     {
          msk_t gate_all = unite(gate_ctrl_[pos], gate_[pos]);
          if (inter(gate_all, cur_bad)
              || !submask(inter(gate_all, locals_), cur_cluster)) {
               return false;
          }

          if (GateType(pos) == 0) {
               return submask(gate_[pos], locals_);
          }
          else {
               return true;
          }
     }
};

#endif  // SCHEDULER_CLUSTER_SCHEDULER
