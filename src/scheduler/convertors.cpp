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

#include "convertors.h"

#include <algorithm>
#include <glog/logging.h>

using std::sort;
using std::unique;

vector<id_num_t> MskToIds(const msk_t msk, const vector<id_num_t> &pos_to_id) {
  vector<id_num_t> ans;
  for (int i = 0; (static_cast<msk_t>(1) << i) <= msk; ++i) {
    if (test_bit(msk, i)) {
      ans.push_back(pos_to_id[i]);
    }
  }

  return ans;
}

msk_t IdsToMsk(const vector<id_num_t> &vec, const map<id_num_t, int> &id_to_pos) {
  msk_t msk = 0;
  for (id_num_t i : vec) {
    set_bit(msk, id_to_pos.find(i)->second);
  }
  return msk;
}

tuple<vector<id_num_t>, map<id_num_t, int>>
CalcPos(const vector<vector<id_num_t>> &_gate, const vector<vector<id_num_t>> &_gate_ctrl,
        const vector<id_num_t> &_locals, const vector<id_num_t> &_globals) {
  vector<id_num_t> pos_to_id;
  for (auto &i : _gate) {
    pos_to_id.insert(pos_to_id.end(), i.begin(), i.end());
  }
  for (auto &i : _gate_ctrl) {
    pos_to_id.insert(pos_to_id.end(), i.begin(), i.end());
  }
  pos_to_id.insert(pos_to_id.end(), _locals.begin(), _locals.end());
  pos_to_id.insert(pos_to_id.end(), _globals.begin(), _globals.end());

  sort(pos_to_id.begin(), pos_to_id.end());
  pos_to_id.erase(unique(pos_to_id.begin(), pos_to_id.end()), pos_to_id.end());

  CHECK(pos_to_id.size() < 64) << "CalcPos(): scheduling with 64 or more qubits is not yet supported";

  map<id_num_t, int> id_to_pos;
  for (size_t i = 0; i < pos_to_id.size(); ++i) {
    id_to_pos[pos_to_id[i]] = static_cast<int>(i);
  }

  return make_tuple(pos_to_id, id_to_pos);
}

tuple<vector<msk_t>, vector<msk_t>>
CalcGates(const vector<vector<id_num_t>> &_gate, const vector<vector<id_num_t>> &_gate_ctrl,
          const map<id_num_t, int> &id_to_pos) {
  vector<msk_t> gate(_gate.size());
  vector<msk_t> gate_ctrl(_gate.size());

  for (size_t i = 0; i < _gate.size(); ++i) {
    gate[i] = IdsToMsk(_gate[i], id_to_pos);
    gate_ctrl[i] = IdsToMsk(_gate_ctrl[i], id_to_pos);
  }

  return make_tuple(gate, gate_ctrl);
}
