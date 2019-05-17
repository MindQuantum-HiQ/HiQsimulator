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

#include <glog/logging.h>

#include <algorithm>
#include <iterator>

std::vector<id_num_t> MskToIds(const msk_t msk,
                               const std::vector<id_num_t> &pos_to_id)
{
     std::vector<id_num_t> ans;
     for (msk_t i = 0; (static_cast<msk_t>(1) << i) <= msk; ++i) {
          if (test_bit(msk, i)) {
               ans.push_back(pos_to_id[i]);
          }
     }

     return ans;
}

msk_t IdsToMsk(const std::vector<id_num_t> &vec,
               const std::map<id_num_t, int> &id_to_pos)
{
     msk_t msk = 0;
     for (id_num_t i: vec) {
          set_bit(msk, id_to_pos.find(i)->second);
     }
     return msk;
}

std::tuple<std::vector<id_num_t>, std::map<id_num_t, int>> CalcPos(
    const std::vector<std::vector<id_num_t>> &gate,
    const std::vector<std::vector<id_num_t>> &gate_ctrl,
    const std::vector<id_num_t> &locals, const std::vector<id_num_t> &globals)
{
     std::vector<id_num_t> pos_to_id;
     for (auto &i: gate) {
          std::copy(i.begin(), i.end(), std::back_inserter(pos_to_id));
     }
     for (auto &i: gate_ctrl) {
          std::copy(i.begin(), i.end(), std::back_inserter(pos_to_id));
     }
     std::copy(locals.begin(), locals.end(), std::back_inserter(pos_to_id));
     std::copy(globals.begin(), globals.end(), std::back_inserter(pos_to_id));

     std::sort(pos_to_id.begin(), pos_to_id.end());
     pos_to_id.erase(std::unique(pos_to_id.begin(), pos_to_id.end()),
                     pos_to_id.end());

     CHECK(pos_to_id.size() < 64)
         << "CalcPos(): scheduling with 64 or more qubits is not yet supported";

     std::map<id_num_t, int> id_to_pos;
     for (size_t i = 0; i < pos_to_id.size(); ++i) {
          id_to_pos[pos_to_id[i]] = static_cast<int>(i);
     }

     return std::make_tuple(pos_to_id, id_to_pos);
}

std::tuple<std::vector<msk_t>, std::vector<msk_t>> CalcGates(
    const std::vector<std::vector<id_num_t>> &gate,
    const std::vector<std::vector<id_num_t>> &gate_ctrl,
    const std::map<id_num_t, int> &id_to_pos)
{
     std::vector<msk_t> gate_new(gate.size());
     std::vector<msk_t> gate_ctrl_new(gate.size());

     for (size_t i = 0; i < gate.size(); ++i) {
          gate_new[i] = IdsToMsk(gate[i], id_to_pos);
          gate_ctrl_new[i] = IdsToMsk(gate_ctrl[i], id_to_pos);
     }

     return std::make_tuple(gate_new, gate_ctrl_new);
}
