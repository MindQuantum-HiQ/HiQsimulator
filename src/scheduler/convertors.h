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

#ifndef SCHEDULER_FUNCS_H
#define SCHEDULER_FUNCS_H

#include <vector>
#include <map>
#include <tuple>

#include "definitions.h"

using std::vector;
using std::map;
using std::tuple;

vector<id_num_t> MskToIds(msk_t msk, const vector<id_num_t> &pos_to_id);

msk_t IdsToMsk(const vector<id_num_t> &vec, const map<id_num_t, int> &id_to_pos);

tuple<vector<id_num_t>, map<id_num_t, int>>
CalcPos(const vector<vector<id_num_t>> &_gate, const vector<vector<id_num_t>> &_gate_ctrl,
        const vector<id_num_t> &_locals, const vector<id_num_t> &_globals);

tuple<vector<msk_t>, vector<msk_t>>
CalcGates(const vector<vector<id_num_t>> &_gate, const vector<vector<id_num_t>> &_gate_ctrl,
          const map<id_num_t, int> &id_to_pos);

#endif // SCHEDULER_FUNCS_H
