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

#include <map>
#include <tuple>
#include <vector>

#include "definitions.h"

std::vector<id_num_t> MskToIds(msk_t msk,
                               const std::vector<id_num_t> &pos_to_id);

msk_t IdsToMsk(const std::vector<id_num_t> &vec,
               const std::map<id_num_t, int> &id_to_pos);

std::tuple<std::vector<id_num_t>, std::map<id_num_t, int>> CalcPos(
    const std::vector<std::vector<id_num_t>> &gate,
    const std::vector<std::vector<id_num_t>> &gate_ctrl,
    const std::vector<id_num_t> &locals, const std::vector<id_num_t> &globals);

std::tuple<std::vector<msk_t>, std::vector<msk_t>> CalcGates(
    const std::vector<std::vector<id_num_t>> &gate,
    const std::vector<std::vector<id_num_t>> &gate_ctrl,
    const std::map<id_num_t, int> &id_to_pos);

#endif  // SCHEDULER_FUNCS_H
