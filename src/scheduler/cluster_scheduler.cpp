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

#include "cluster_scheduler.h"

#include <vector>
#include <set>

#include <glog/logging.h>
#include "definitions.h"
#include "convertors.h"

using std::vector;
using std::set;

ClusterScheduler::ClusterScheduler(const vector<vector<id_num_t>> _gate, const vector<vector<id_num_t>> _gate_ctrl,
                                   const vector<bool> _gate_diag, const vector<id_num_t> _locals,
                                   const vector<id_num_t> _globals, const int _cluster_size) :
    ClusterSize(_cluster_size), gate_diag(_gate_diag) {
  CHECK(_gate.size() == _gate_ctrl.size() && _gate.size() == _gate_diag.size()) << "ctor():";
  tie(pos_to_id, id_to_pos) = CalcPos(_gate, _gate_ctrl, _locals, _globals);
  tie(gate, gate_ctrl) = CalcGates(_gate, _gate_ctrl, id_to_pos);
  locals = IdsToMsk(_locals, id_to_pos);
  globals = IdsToMsk(_globals, id_to_pos);
}

vector<int> ClusterScheduler::ScheduleCluster() {
  VLOG(3) << "ScheduleCluster(): started scheduling cluster: gates_no = " << gate.size();

  if (gate.empty()) {
    VLOG(3) << "ScheduleCluster(): empty gates list, exiting";
    return {};
  }

  best_ans = 0;
  best_cluster = 0;
  for (int i = 0; i < static_cast<int>(gate.size()); ++i) {
    auto cur_locals = inter(locals, unite(gate[i], gate_ctrl[i]));
    if (count_bits(cur_locals) <= ClusterSize) {
      Rec(cur_locals, 1);
    }
  }

  if (best_ans != 0) {
    auto ans = GetComamndsFromMsk();
    VLOG(3) << "ScheduleCluster(): finished scheduling cluster: expected qubits_no = " << count_bits(best_cluster)
              << "; expected commands_no = " << ans.size();
    return ans;
  }

  auto ans = GetCommandsFromLocals();
  if (!ans.empty()) {
    VLOG(3) << "ScheduleCluster(): found cluster with huge gate";
    return ans;
  }

  VLOG(3) << "ScheduleCluster(): no available clusters found, exiting";
  return {};
}

void ClusterScheduler::Rec(msk_t cur_cluster, msk_t bit) {
  CalcCombination(cur_cluster);

  if (bit > locals) {
    return;
  }

  if (inter(used[cur_cluster], bit)) {
    return;
  }
  used[cur_cluster] = unite(used[cur_cluster], bit);

  if (submask(bit, locals) && !inter(bit, cur_cluster) && count_bits(cur_cluster) < ClusterSize) {
    Rec(unite(cur_cluster, bit), bit << 1);
  }

  Rec(cur_cluster, bit << 1);
}
