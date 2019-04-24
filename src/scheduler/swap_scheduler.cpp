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

using std::cout;
using std::endl;
using std::accumulate;

SwapScheduler::SwapScheduler(const vector<vector<id_num_t>> _gate, const vector<vector<id_num_t>> _gate_ctrl,
                             const vector<bool> _gate_diag, const int _num_splits, const int _num_locals, bool fuse) :
    NumSplits(_num_splits), NumLocals(_num_locals), gate_diag(_gate_diag),
    gate_weight(_gate.size(), 1) {
  CHECK(_gate.size() == _gate_ctrl.size() && _gate.size() == _gate_diag.size()) << "ctor():";
  tie(pos_to_id, id_to_pos) = CalcPos(_gate, _gate_ctrl, {}, {});
  tie(gate, gate_ctrl) = CalcGates(_gate, _gate_ctrl, id_to_pos);

  if(fuse) {
    FuseSingleQubitGates();
  }
}

void SwapScheduler::FuseSingleQubitGates() {
  for (int i = static_cast<int>(gate.size()) - 1; i >= 0; --i) {
    bool remove = false;
    if (count_bits(unite(gate[i], gate_ctrl[i])) == 1) {
      if (GateType(i) == 0) {
        remove = (FuseSingleQubitToNextInterGate(i) || FuseSingleQubitToPrevInterGate(i));
      } else {
        remove = (FuseSingleQubitToPrevInterGate(i) || FuseSingleQubitToNextInterGate(i));
      }
    }
    if (remove) {
      gate.erase(gate.begin() + i);
      gate_ctrl.erase(gate_ctrl.begin() + i);
      gate_diag.erase(gate_diag.begin() + i);
      gate_weight.erase(gate_weight.begin() + i);
    }
  }
}

vector<id_num_t> SwapScheduler::ScheduleSwap() {
  VLOG(1) << "ScheduleSwap(): started scheduling swap: gates_no = " << gate.size() << " real gates_no = "
            << accumulate(gate_weight.begin(), gate_weight.end(), 0);

  if (gate.empty()) {
    VLOG(1) << "ScheduleSwap(): empty gates list, exiting";
    return {};
  }

  best_ans = 0;
  best_locals = 0;
  int splits_left = Rec(0, 0, 0, 0, NumSplits);
  VLOG(1) << "ScheduleSwap(): finished scheduling swap: expected gates_no = " << best_ans << "; used "
            << NumSplits - splits_left << "/" << NumSplits << " splits";
  return MskToIds(best_locals, pos_to_id);
}

int SwapScheduler::Rec(const int pos, const msk_t cur_locals, const msk_t cur_bad, const int cur_ans, int splits_left) {
  if (cur_ans > best_ans) {
    best_ans = cur_ans;
    best_locals = cur_locals;
  }

  if (pos == static_cast<int>(gate.size())) {
    return splits_left;
  }

  bool can_skip = true;
  int cnt_take = 1;

  bool can_take = CanTake(pos, cur_locals, cur_bad);
  if (can_take) {
    if (GateType(pos) == 0) {
      if (submask(gate[pos], cur_locals)) {
        can_skip = false;
        --cnt_take;
      }
      ++cnt_take;
    } else {
      can_skip = false;
    }
  }

  CHECK(0 < cnt_take && cnt_take <= 2)
  << "Rec(): Internal error: can't do anything with command: ctrl = " << gate_ctrl[pos] << "; qubits = " << gate[pos]
  << "; locals = " << cur_locals << "; bad = " << cur_bad;

  if (splits_left == 0) {
    if (can_take && can_skip) {
      CHECK(cnt_take == 2) << "Rec(): Internal error: branch num";
      can_skip = false;
      --cnt_take;
    }

    if (can_take) {
      CHECK(cnt_take == 1) << "Rec(): Internal error: branch num";
    } else {
      CHECK(can_skip && cnt_take == 1) << "Rec(): Internal error: branch num";
    }

    CHECK(cnt_take == 1) << "Rec(): Internal error: branch num";
  }

  splits_left -= cnt_take - 1;
  CHECK(splits_left >= 0) << "Rec(): Internal error:";

  if (can_skip) {
    int give_splits = splits_left / cnt_take;
    splits_left +=
        Rec(pos + 1, cur_locals, unite(cur_bad, unite(gate[pos], gate_ctrl[pos])), cur_ans, give_splits) -
            give_splits;
    --cnt_take;
  }

  if (can_take) {
    if (GateType(pos) == 0) {
      splits_left = Rec(pos + 1, unite(cur_locals, gate[pos]), cur_bad, cur_ans + gate_weight[pos], splits_left);
      --cnt_take;
    } else {
      splits_left = Rec(pos + 1, cur_locals, cur_bad, cur_ans + gate_weight[pos], splits_left);
      --cnt_take;
    }
  }

  CHECK(cnt_take == 0) << "Rec(): Internal error: invalid branch number";
  return splits_left;
}
