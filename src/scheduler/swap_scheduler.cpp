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

#include "swap_scheduler.h"

#include <glog/logging.h>

#include <iostream>
#include <numeric>

#include "convertors.h"
#include "definitions.h"

SwapScheduler::SwapScheduler(
    const std::vector<std::vector<id_num_t>>& gate,
    const std::vector<std::vector<id_num_t>>& gate_ctrl,
    std::vector<bool> gate_diag, const int num_splits, const int num_locals,
    bool fuse)
    : num_splits_(num_splits),
      num_locals_(num_locals),
      gate_diag_(std::move(gate_diag)),
      gate_weight_(gate.size(), 1),
      best_ans_(0),
      best_locals_(0)
{
     CHECK(gate.size() == gate_ctrl.size() && gate.size() == gate_diag_.size())
         << "ctor():";
     tie(pos_to_id_, id_to_pos_) = CalcPos(gate, gate_ctrl, {}, {});
     tie(gate_, gate_ctrl_) = CalcGates(gate, gate_ctrl, id_to_pos_);

     if (fuse) {
          FuseSingleQubitGates();
     }
}

void SwapScheduler::FuseSingleQubitGates()
{
     for (int i = static_cast<int>(gate_.size()) - 1; i >= 0; --i) {
          bool remove = false;
          if (count_bits(unite(gate_[i], gate_ctrl_[i])) == 1) {
               if (GateType(i) == 0) {
                    remove = (FuseSingleQubitToNextInterGate(i)
                              || FuseSingleQubitToPrevInterGate(i));
               }
               else {
                    remove = (FuseSingleQubitToPrevInterGate(i)
                              || FuseSingleQubitToNextInterGate(i));
               }
          }
          if (remove) {
               gate_.erase(gate_.begin() + i);
               gate_ctrl_.erase(gate_ctrl_.begin() + i);
               gate_diag_.erase(gate_diag_.begin() + i);
               gate_weight_.erase(gate_weight_.begin() + i);
          }
     }
}

std::vector<id_num_t> SwapScheduler::ScheduleSwap()
{
     VLOG(1) << "ScheduleSwap(): started scheduling swap: gates_no = "
             << gate_.size() << " real gates_no = "
             << std::accumulate(gate_weight_.begin(), gate_weight_.end(), 0);

     if (gate_.empty()) {
          VLOG(1) << "ScheduleSwap(): empty gates list, exiting";
          return {};
     }

     best_ans_ = 0;
     best_locals_ = 0;
     int splits_left = Rec(0, 0, 0, 0, num_splits_);
     VLOG(1) << "ScheduleSwap(): finished scheduling swap: expected gates_no = "
             << best_ans_ << "; used " << num_splits_ - splits_left << "/"
             << num_splits_ << " splits";
     return MskToIds(best_locals_, pos_to_id_);
}

int SwapScheduler::Rec(const int pos, const msk_t cur_locals,
                       const msk_t cur_bad, const int cur_ans, int splits_left)
{
     if (cur_ans > best_ans_) {
          best_ans_ = cur_ans;
          best_locals_ = cur_locals;
     }

     if (pos == static_cast<int>(gate_.size())) {
          return splits_left;
     }

     bool can_skip = true;
     int cnt_take = 1;

     bool can_take = CanTake(pos, cur_locals, cur_bad);
     if (can_take) {
          if (GateType(pos) == 0) {
               if (submask(gate_[pos], cur_locals)) {
                    can_skip = false;
                    --cnt_take;
               }
               ++cnt_take;
          }
          else {
               can_skip = false;
          }
     }

     CHECK(0 < cnt_take && cnt_take <= 2)
         << "Rec(): Internal error: can't do anything with command: ctrl = "
         << gate_ctrl_[pos] << "; qubits = " << gate_[pos]
         << "; locals = " << cur_locals << "; bad = " << cur_bad;

     if (splits_left == 0) {
          if (can_take && can_skip) {
               CHECK(cnt_take == 2) << "Rec(): Internal error: branch num";
               can_skip = false;
               --cnt_take;
          }

          if (can_take) {
               CHECK(cnt_take == 1) << "Rec(): Internal error: branch num";
          }
          else {
               CHECK(can_skip && cnt_take == 1)
                   << "Rec(): Internal error: branch num";
          }

          CHECK(cnt_take == 1) << "Rec(): Internal error: branch num";
     }

     splits_left -= cnt_take - 1;
     CHECK(splits_left >= 0) << "Rec(): Internal error:";

     if (can_skip) {
          int give_splits = splits_left / cnt_take;
          splits_left += Rec(pos + 1, cur_locals,
                             unite(cur_bad, unite(gate_[pos], gate_ctrl_[pos])),
                             cur_ans, give_splits)
                         - give_splits;
          --cnt_take;
     }

     if (can_take) {
          if (GateType(pos) == 0) {
               splits_left
                   = Rec(pos + 1, unite(cur_locals, gate_[pos]), cur_bad,
                         cur_ans + gate_weight_[pos], splits_left);
               --cnt_take;
          }
          else {
               splits_left = Rec(pos + 1, cur_locals, cur_bad,
                                 cur_ans + gate_weight_[pos], splits_left);
               --cnt_take;
          }
     }

     CHECK(cnt_take == 0) << "Rec(): Internal error: invalid branch number";
     return splits_left;
}
