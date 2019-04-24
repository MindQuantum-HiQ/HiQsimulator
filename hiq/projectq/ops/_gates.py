#   Copyright 2019 <Huawei Technologies Co., Ltd>
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from projectq.backends import ResourceCounter
from projectq.meta import get_control_count
from projectq.ops import FastForwardingGate, ClassicalInstructionGate


class MetaSwapGate(FastForwardingGate):
    """ Command to the simulator to swap global and local qubits """

    def __str__(self):
        return "SW"


MetaSwap = MetaSwapGate()


class AllocateQuregGate(ClassicalInstructionGate):
    def __init__(self, init=0):
        self.interchangeable_qubit_indices = []
        self._init = init

    @property
    def init(self):
        return self._init

    def __str__(self):
        return "AllocateQureg"


def _hiq_add_cmd(resource_counter, cmd):
    """
    Add a gate to the count.
    """
    if isinstance(cmd.gate, AllocateQuregGate):
        for qureg in cmd.qubits:
            for qubit in qureg:
                resource_counter._active_qubits += 1
                resource_counter._depth_of_qubit[qubit.id] = 0

        # this is original ProjectQ's code copied here from _add_cmd()
        resource_counter.max_width = max(resource_counter.max_width, resource_counter._active_qubits)

        ctrl_cnt = get_control_count(cmd)
        gate_description = (cmd.gate, ctrl_cnt)
        gate_class_description = (cmd.gate.__class__, ctrl_cnt)

        try:
            resource_counter.gate_counts[gate_description] += 1
        except KeyError:
            resource_counter.gate_counts[gate_description] = 1

        try:
            resource_counter.gate_class_counts[gate_class_description] += 1
        except KeyError:
            resource_counter.gate_class_counts[gate_class_description] = 1
    else:
        resource_counter._old_add_cmd(cmd)

ResourceCounter._old_add_cmd = ResourceCounter._add_cmd
ResourceCounter._add_cmd = _hiq_add_cmd