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

#include <vector>
#include <iostream>
#include <map>
#include <glog/logging.h>

#include "definitions.h"

using std::vector;
using std::map;

class ClusterScheduler {
 public:
  // Finds the largest subset of gates than can be fused into one cluster
  // Uses backtracking on qubits ids
  // Works in O(C(num_qubits, _cluster_size) * num_gates) time, O(C(num_qubits, _cluster_size)) memory
  vector<int> ScheduleCluster();

  ClusterScheduler(vector<vector<id_num_t>> _gate, vector<vector<id_num_t>> _gate_ctrl, vector<bool> _gate_diag,
                   vector<id_num_t> _locals, vector<id_num_t> _globals, int _cluster_size);

  ~ClusterScheduler() {}

 private:
  const int ClusterSize;
  int best_ans;
  msk_t best_cluster;
  msk_t locals;
  msk_t globals;
  map<msk_t, msk_t> used;
  vector<msk_t> gate, gate_ctrl;
  vector<bool> gate_diag;
  vector<id_num_t> pos_to_id;
  map<id_num_t, int> id_to_pos;

  // Generates qubits combinations, containing no more than ClusterSize qubits
  void Rec(msk_t cur_cluster, msk_t bit);

  // Tries to improve answer by given qubits combination
  inline void CalcCombination(msk_t cur_cluster) {
    if (used[cur_cluster] != 0) {
      return;
    }
    msk_t cur_bad = 0;
    int cur_ans = 0;

    for (int i = 0; i < static_cast<int>(gate.size()); ++i) {
      if (CanTake(i, cur_cluster, cur_bad)) {
        ++cur_ans;
      } else {
        cur_bad = unite(cur_bad, unite(gate[i], gate_ctrl[i]));
      }
    }

    if (cur_ans > best_ans || (cur_ans == best_ans && count_bits(cur_cluster) < count_bits(best_cluster))) {
      best_ans = cur_ans;
      best_cluster = cur_cluster;
    }
  }

  // Returns gates to be used in current cluster
  inline vector<int> GetComamndsFromMsk() const {
    msk_t cur_bad = 0;

    vector<int> ans;
    for (int i = 0; i < static_cast<int>(gate.size()); ++i) {
      if (CanTake(i, best_cluster, cur_bad)) {
        ans.push_back(i);
      } else {
        cur_bad = unite(cur_bad, unite(gate[i], gate_ctrl[i]));
      }
    }

    CHECK(static_cast<int>(ans.size()) == best_ans) << "GetComamndsFromMsk(): Internal error: sizes mismatch";
    return ans;
  }

  // Returns huge gate to be used in current cluster if possible
  inline vector<int> GetCommandsFromLocals() const {
    msk_t cur_bad = 0;

    for (int i = 0; i < static_cast<int>(gate.size()); ++i) {
      if (CanTake(i, locals, cur_bad)) {
        return {i};
      } else {
        cur_bad = unite(cur_bad, unite(gate[i], gate_ctrl[i]));
      }
    }

    return {};
  }

  // Returns pos-th gate type.
  // 0 - not diagonal
  // 1 - diagonal
  inline int GateType(const int pos) const {
    return gate_diag[pos];
  }

  // Determines if is it possible to apply pos-th gate in given situation.
  inline bool CanTake(const int pos, const msk_t cur_cluster, const msk_t cur_bad) const {
    msk_t gate_all = unite(gate_ctrl[pos], gate[pos]);
    if (inter(gate_all, cur_bad) || !submask(inter(gate_all, locals), cur_cluster)) {
      return false;
    }

    if (GateType(pos) == 0) {
      return submask(gate[pos], locals);
    } else {
      return true;
    }
  }
};

#endif // SCHEDULER_CLUSTER_SCHEDULER
