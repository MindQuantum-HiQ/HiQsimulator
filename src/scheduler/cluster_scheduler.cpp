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

#include <glog/logging.h>

#include <vector>

#include "convertors.h"
#include "definitions.h"

ClusterScheduler::ClusterScheduler(
    const std::vector<std::vector<id_num_t>>& gate,
    const std::vector<std::vector<id_num_t>>& gate_ctrl,
    std::vector<bool> gate_diag, const std::vector<id_num_t>& locals,
    const std::vector<id_num_t>& globals, const int cluster_size)
    : cluster_size_(cluster_size),
      best_ans_(0),
      best_cluster_(0),
      gate_diag_(std::move(gate_diag))
{
     CHECK(gate.size() == gate_ctrl.size() && gate.size() == gate_diag_.size())
         << "ctor():";
     tie(pos_to_id_, id_to_pos_) = CalcPos(gate, gate_ctrl, locals, globals);
     tie(gate_, gate_ctrl_) = CalcGates(gate, gate_ctrl, id_to_pos_);
     locals_ = IdsToMsk(locals, id_to_pos_);
     globals_ = IdsToMsk(globals, id_to_pos_);
}

std::vector<int> ClusterScheduler::ScheduleCluster()
{
     VLOG(3) << "ScheduleCluster(): started scheduling cluster: gates_no = "
             << gate_.size();

     if (gate_.empty()) {
          VLOG(3) << "ScheduleCluster(): empty gates list, exiting";
          return {};
     }

     best_ans_ = 0;
     best_cluster_ = 0;
     for (int i = 0; i < static_cast<int>(gate_.size()); ++i) {
          auto cur_locals = inter(locals_, unite(gate_[i], gate_ctrl_[i]));
          if (count_bits(cur_locals) <= cluster_size_) {
               Rec(cur_locals, 1);
          }
     }

     if (best_ans_ != 0) {
          auto ans = GetComamndsFromMsk();
          VLOG(3) << "ScheduleCluster(): finished scheduling cluster: expected "
                     "qubits_no = "
                  << count_bits(best_cluster_)
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

void ClusterScheduler::Rec(msk_t cur_cluster, msk_t bit)
{
     CalcCombination(cur_cluster);

     if (bit > locals_) {
          return;
     }

     if (inter(used_[cur_cluster], bit)) {
          return;
     }
     used_[cur_cluster] = unite(used_[cur_cluster], bit);

     if (submask(bit, locals_) && !inter(bit, cur_cluster)
         && count_bits(cur_cluster) < cluster_size_) {
          Rec(unite(cur_cluster, bit), bit << 1UL);
     }

     Rec(cur_cluster, bit << 1UL);
}
